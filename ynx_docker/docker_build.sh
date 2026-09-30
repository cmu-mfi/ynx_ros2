cd ..
docker build --platform linux/arm64 -f ynx_docker/Dockerfile -t ynx_ros2 .
cd ynx_docker
./acu_save_img -i ynx_ros2 -v
