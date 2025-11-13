import os
import urllib
import traceback
import time
import sys
import numpy as np
import cv2
from rknn.api import RKNN
from flask import Flask, Response


ONNX_MODEL = 'yolov5s_relu.onnx'
RKNN_MODEL = 'yolov5s_relu.rknn'
DATASET = './dataset.txt'

QUANTIZE_ON = True

OBJ_THRESH = 0.25
NMS_THRESH = 0.45
IMG_SIZE = 640

CLASSES = ("person", "bicycle", "car", "motorbike ", "aeroplane ", "bus ", "train", "truck ", "boat", "traffic light","fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog ", "horse ", "sheep", "cow", "elephant","bear", "zebra ", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite","baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife ","spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza ", "donut", "cake", "chair", "sofa","pottedplant", "bed", "diningtable", "toilet ", "tvmonitor", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave","oven ", "toaster", "sink", "refrigerator ", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush")

def xywh2xyxy(x):
    y = np.copy(x)
    y[:, 0] = x[:, 0] - x[:, 2] / 2  
    y[:, 1] = x[:, 1] - x[:, 3] / 2  
    y[:, 2] = x[:, 0] + x[:, 2] / 2  
    y[:, 3] = x[:, 1] + x[:, 3] / 2  
    return y

def process(input, mask, anchors):

    anchors = [anchors[i] for i in mask]
    grid_h, grid_w = map(int, input.shape[0:2])

    box_confidence = input[..., 4]
    box_confidence = np.expand_dims(box_confidence, axis=-1)

    box_class_probs = input[..., 5:]

    box_xy = input[..., :2]*2 - 0.5

    col = np.tile(np.arange(0, grid_w), grid_w).reshape(-1, grid_w)
    row = np.tile(np.arange(0, grid_h).reshape(-1, 1), grid_h)
    col = col.reshape(grid_h, grid_w, 1, 1).repeat(3, axis=-2)
    row = row.reshape(grid_h, grid_w, 1, 1).repeat(3, axis=-2)
    grid = np.concatenate((col, row), axis=-1)
    box_xy += grid
    box_xy *= int(IMG_SIZE/grid_h)

    box_wh = pow(input[..., 2:4]*2, 2)
    box_wh = box_wh * anchors

    box = np.concatenate((box_xy, box_wh), axis=-1)

    return box, box_confidence, box_class_probs

def filter_boxes(boxes, box_confidences, box_class_probs):
    boxes = boxes.reshape(-1, 4)
    box_confidences = box_confidences.reshape(-1)
    box_class_probs = box_class_probs.reshape(-1, box_class_probs.shape[-1])

    _box_pos = np.where(box_confidences >= OBJ_THRESH)
    boxes = boxes[_box_pos]
    box_confidences = box_confidences[_box_pos]
    box_class_probs = box_class_probs[_box_pos]

    class_max_score = np.max(box_class_probs, axis=-1)
    classes = np.argmax(box_class_probs, axis=-1)
    _class_pos = np.where(class_max_score >= OBJ_THRESH)

    boxes = boxes[_class_pos]
    classes = classes[_class_pos]
    scores = (class_max_score* box_confidences)[_class_pos]

    return boxes, classes, scores

def nms_boxes(boxes, scores):
    x = boxes[:, 0]
    y = boxes[:, 1]
    w = boxes[:, 2] - boxes[:, 0]
    h = boxes[:, 3] - boxes[:, 1]
    areas = w * h
    order = scores.argsort()[::-1]
    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)

        xx1 = np.maximum(x[i], x[order[1:]])
        yy1 = np.maximum(y[i], y[order[1:]])
        xx2 = np.minimum(x[i] + w[i], x[order[1:]] + w[order[1:]])
        yy2 = np.minimum(y[i] + h[i], y[order[1:]] + h[order[1:]])

        w1 = np.maximum(0.0, xx2 - xx1 + 0.00001)
        h1 = np.maximum(0.0, yy2 - yy1 + 0.00001)
        inter = w1 * h1

        ovr = inter / (areas[i] + areas[order[1:]] - inter)
        inds = np.where(ovr <= NMS_THRESH)[0]
        order = order[inds + 1]
    keep = np.array(keep)
    return keep

def yolov5_post_process(input_data):
    masks = [[0, 1, 2], [3, 4, 5], [6, 7, 8]]
    anchors = [[10, 13], [16, 30], [33, 23], [30, 61], [62, 45],
               [59, 119], [116, 90], [156, 198], [373, 326]]
    boxes, classes, scores = [], [], []
    for input, mask in zip(input_data, masks):
        b, c, s = process(input, mask, anchors)
        b, c, s = filter_boxes(b, c, s)
        boxes.append(b)
        classes.append(c)
        scores.append(s)
    boxes = np.concatenate(boxes)
    boxes = xywh2xyxy(boxes)
    classes = np.concatenate(classes)
    scores = np.concatenate(scores)
    nboxes, nclasses, nscores = [], [], []
    for c in set(classes):
        inds = np.where(classes == c)
        b = boxes[inds]
        c = classes[inds]
        s = scores[inds]
        keep = nms_boxes(b, s)
        nboxes.append(b[keep])
        nclasses.append(c[keep])
        nscores.append(s[keep])
    if not nclasses and not nscores:
        return None, None, None
    boxes = np.concatenate(nboxes)
    classes = np.concatenate(nclasses)
    scores = np.concatenate(nscores)
    return boxes, classes, scores

def draw(frame_shape, boxes, scores, classes):
    overlay = np.zeros(frame_shape, dtype=np.uint8) 
    for box, score, cl in zip(boxes, scores, classes):
        x1, y1, x2, y2 = map(int, box)
        color = (0, 255, 0)
        cv2.rectangle(overlay, (x1, y1), (x2, y2), color, 2)
        class_name = CLASSES[int(cl)] if int(cl) < len(CLASSES) else f'class_{int(cl)}'
        label = f'{class_name} {score:.2f}'
        (text_w, text_h), baseline = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)
        y1_text = max(y1 - text_h - 5, 0)
        cv2.rectangle(overlay, (x1, y1_text), (x1 + text_w, y1), color, -1)
        cv2.putText(overlay, label, (x1, y1 - 5),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)
    return overlay


def letterbox(im, new_shape=(640, 640), color=(0, 0, 0)):  
    shape = im.shape[:2]  
    if isinstance(new_shape, int):
        new_shape = (new_shape, new_shape)
    r = min(new_shape[0] / shape[0], new_shape[1] / shape[1])
    ratio = r, r  
    new_unpad = int(round(shape[1] * r)), int(round(shape[0] * r))
    dw, dh = new_shape[1] - new_unpad[0], new_shape[0] - new_unpad[1]  
    dw /= 2 
    dh /= 2
    if shape[::-1] != new_unpad:  
        im = cv2.resize(im, new_unpad, interpolation=cv2.INTER_LINEAR)
    top, bottom = int(round(dh - 0.1)), int(round(dh + 0.1))
    left, right = int(round(dw - 0.1)), int(round(dw + 0.1))
    im = cv2.copyMakeBorder(im, top, bottom, left, right, cv2.BORDER_CONSTANT, value=color)  
    return im, ratio, (dw, dh)

if __name__ == '__main__':

    rknn = RKNN(verbose=False)
    print('Config model')
    rknn.config(mean_values=[[0, 0, 0]], std_values=[[255, 255, 255]], target_platform='rk3588')
    print('done')
    print('Loading model')
    ret = rknn.load_onnx(model=ONNX_MODEL)
    if ret != 0:
        print('Load model failed!')
        exit(ret)
    print('done')
    print('Building model')
    ret = rknn.build(do_quantization=QUANTIZE_ON, dataset=DATASET)
    if ret != 0:
        print('Build model failed!')
        exit(ret)
    print('done')
    print(' Init runtime environment')
    ret = rknn.init_runtime(target='rk3588') 
    if ret != 0:
        print('Init runtime environment failed!')
        exit(ret)
    print('done')
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("Cannot open camera!")
        exit()
    print("Open successfully!")
    last_detect_time = 0  
    DETECT_INTERVAL = 0.5   
    cv2.namedWindow("YOLOv5 RKNN Detection", cv2.WINDOW_NORMAL)

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Error!")
            break

        display_frame = frame.copy()
        current_time = time.time()
        if current_time - last_detect_time >= DETECT_INTERVAL:
            last_detect_time = current_time
            img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            img = cv2.resize(img, (IMG_SIZE, IMG_SIZE))
            print(" Running YOLOv5 detection...")
            outputs = rknn.inference(inputs=[img])
            input0_data = outputs[0].reshape([3, -1] + list(outputs[0].shape[-2:]))
            input1_data = outputs[1].reshape([3, -1] + list(outputs[1].shape[-2:]))
            input2_data = outputs[2].reshape([3, -1] + list(outputs[2].shape[-2:]))
            input_data = [
                np.transpose(input0_data, (2, 3, 0, 1)),
                np.transpose(input1_data, (2, 3, 0, 1)),
                np.transpose(input2_data, (2, 3, 0, 1))
            ]

            boxes, classes, scores = yolov5_post_process(input_data)

            if boxes is not None:
                overlay = draw(display_frame.shape, boxes, scores, classes)
                print(f"Successfully!, {len(boxes)} objects detected.")
            else:
                
                print("Cannot detect any objects!")

        display_frame = cv2.addWeighted(display_frame, 1.0, overlay, 0.8, 0)
        latest_frame = display_frame.copy()   
        cv2.imshow("YOLOv5 RKNN Detection", display_frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("Ending")
            break

    cap.release()
    cv2.destroyAllWindows()
    rknn.release()