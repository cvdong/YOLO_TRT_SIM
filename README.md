# YOLO_TRT_SIM

 一套代码同时支持 V5,V6,V7,V8 TRT推理 ™️，前后处理均由CUDA核函数实现 :rocket:

 该REPO功能描述：
 - 支持onnx转TRT FP32 FP16 INT8 engine;
 - 支持动态batch 推理；
 - 支持image和video 推理；
 - 支持多路多线程并行推理；
 - 仅依赖opencv和tensorrt;
 - 支持YOLOV5 V6 V7 V8 推理；
 - 友好的封装格式，便于学习

### MY ENVIRONMENT

- cuda 11.7
- cudnn 8.4
- opencv 4.6
- tensorrt 8.4

### ONNX
pipeline: pt-->onnx-->engine

YOLOV5 onnx:

```
https://github.com/ultralytics/yolov5
python export.py --weights weights/yolov5s.pt --simplify
```
![](./workspace/yolov5s_onnx_cut.png)

YOLOV6 onnx:
```
https://github.com/meituan/YOLOv6
python deploy/ONNX/export_onnx.py --weights weights/yolov6s.pt --simplify
```
![](./workspace/yolov6s_onnx_cut.png)

YOLOV7 onnx:
```
https://github.com/WongKinYiu/yolov7
python export.py --weights weights/yolov7s.pt --grid --simplify 
```
![](./workspace/yolov7s_onnx_cut.png)

YOLOV8 onnx:
```


```
![](./workspace/yolov8s_onnx_cut.png)

### Engine


### 教程




## 个人思考： 




![](./workspace/result/zidane.jpg)
REPO参考：[https://github.com/shouxieai/tensorRT_Pro](https://github.com/shouxieai/tensorRT_Pro)

DULAO YYDS :heartpulse: 