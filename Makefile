build:gen
	cd build && ninja

gen:
	cmake -G Ninja -B build -S .

clean:
	rm -rf build
	rm -f CMakeCache.txt
	rm -f CMakeFiles
	rm -f cmake_install.cmake
	rm -f CMakeOutput.log
	rm -f CMakeError.log
	rm -rf .cache
	
run:
	cd build && ./iot
	@echo "Run complete"
	
test_code:
	mkdir -p build
	g++ -c test/test_code.cpp -o build/test_code.o
	g++ -c src/Src/coder.cpp -o build/coder.o
	g++ build/coder.o build/test_code.o -o build/test_code
	cd build && ./test_code
	
test_camera:
	mkdir -p build
	g++ -I src/Inc -I/usr/include/opencv4 -lopencv_gapi -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_cvv -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hdf -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_signal -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_text -lopencv_plot -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_wechat_qrcode -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core test/test_camera.cpp -o build/test_camera
	cd build && ./test_camera ../test/road.jpg
