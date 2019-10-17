# vehicle-tracker
[Demo Video](https://youtu.be/kf5vGxIsrzU)

### Summary
This is a simple example on detect and track trucks using opencv and dlib on Qt. The idea is to have an object detector to detect the trucks, then using dlib correlation tracker to track the truck. This does not mean that dlib tracker is better than opencv tracker. I just happend to find some tutorials about the dlib tracker. The default object detector is the YOLO network trained on COCO dataset that I downloaded from the leanopencv tutorial.

Every N frames, the object detector will be run and all detected and untracked trucks will be added to the track list. To check if a truck is tracked or not, the IOU of the newly detected truck bounding box and the existing tracked bounding boxes are calculated and compared to a threshold. If the calculated IOU higher than the threshold then the truck is consider tracked.

Every M frames, the displacement of the bounding box of each truck is calculated. If the displacement is under a threshold, the truck is consider stopped and that particular frame will be noted down to calculate the waiting time of the truck.

Every frames the tracked trucks are updated with the new frame.

The GUI is just a simple mediaplayer with a button to open the file in QML.

In my particular test video I also cropped some of the left and right side of the video as they are not needed for detecting the trucks I am interested in.

SSD networks are included in the project but they are not working as expected so I do not use them in the final version.

### Library:
- [opencv](https://opencv.org)
- [Qt](https://www.qt.io)
- [dlib](http://dlib.net)

### Reference
- [pyimagesearch multiobject tracking with dlib](https://www.pyimagesearch.com/2018/10/29/multi-object-tracking-with-dlib/)
- [pyimagesearch opencv people counter](https://www.pyimagesearch.com/2018/08/13/opencv-people-counter/)
- [learnopencv object-detection using YOLOv3](https://www.learnopencv.com/deep-learning-based-object-detection-using-yolov3-with-opencv-python-c/)
- [sample video filter application](https://github.com/stephenquan/MyVideoFilterApp)
- [mobilenet-SSD](https://github.com/chuanqi305/MobileNet-SSD)
- [SSD](https://github.com/weiliu89/caffe/tree/ssd)

### TODO
Add detection for when a truck leave the screen and clean up the tracker accordingly

check why the SSD is not working as expected

Add buttons to GUI to choose network

Try OpenCV tracker. There will be less convert forth and back.

library installation instruction
