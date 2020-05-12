#include <vector>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <typeinfo>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <sstream>


#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#ifdef BAZEL_BUILD
#include "examples/protos/my_protobuf.grpc.pb.h"
#else
#include "my_protobuf.grpc.pb.h"
#endif

typedef char byte;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using my_protobuf::ImageType;
using my_protobuf::TextType;
using my_protobuf::JSONType;
using my_protobuf::RequestImageType;
using my_protobuf::ReplyImageType;
using my_protobuf::transactionTest;

// Logic and data behind the server's behavior.
class transactionTestServiceImpl final : public transactionTest::Service {
  Status ItextOtext(ServerContext* context, const TextType* request,
                          TextType* reply) override {
    reply->set_data(request->data());
    return Status::OK;
  }

  Status IimageOimage(ServerContext* context, const ImageType* request, 
                            ImageType* reply) override {

    cv::Mat img;
    cv::VideoCapture cap;
    cap.open("/dev/video10"); 
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 0x7FFFFFFF);          // working
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 0x7FFFFFFF);         // working
    
    if (cap.isOpened())
        cap >> img;
        cv::imwrite("./image_log/temp.jpg", img);

    // load image from local system
    std::ifstream ifs("./image_log/temp.jpg", std::ios::binary | std::ios::in);

    // calculate file size
    ifs.seekg(0, ifs.end);
    int length = (int)ifs.tellg();
    ifs.seekg(0, ifs.beg);

    // read image in buffer
    char* buffer = new char [length];
    ifs.read((char*)buffer, length);
    ifs.close();

    // set format & data
    reply->set_format(".jpg");
    reply->set_data(std::string(buffer, length));
    

    return Status::OK;
  }

  Status IjsonOjson(ServerContext* context, const JSONType* request, JSONType* reply) override {
    reply->set_data(request->data());

    return Status::OK;
  }

  Status IReqITORepIT(ServerContext* context, const RequestImageType* request, ReplyImageType* reply) override {
    reply->set_name(request->name());
    
    //set local image path
    std::stringstream ss;
    ss << "../../image_log/" << request->name() << ".jpg"; 

    // door open
    int fd = open("/sys/class/gpio/gpio12/value", O_WRONLY);
    char* temp = new char [1];
    temp[0] = '0';
    write(fd, temp, 1);

    // gpio pin poling setup
    char* door;
    char* lock;
    int fd42;
    int fd39;
    
    sleep(3);

    // wait until door close
    while(true){
      fd42 = open("/sys/class/gpio/gpio42/value", O_RDONLY);
      fd39 = open("/sys/class/gpio/gpio39/value", O_RDONLY);
      door = new char[1];
      lock = new char[1];
      read(fd42, lock, 1);
      read(fd39, door, 1);


      if (lock[0] == '1' && door[0] == '0')
          break;

      delete door;
      delete lock;
      close(fd39);
      close(fd42);
      sleep(1);

    } 
    close(fd);
    delete temp;

    // get image frame from camera
    cv::Mat img;
    cv::VideoCapture cap;
    cap.open("/dev/video10"); 
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 0x7FFFFFFF);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 0x7FFFFFFF);
    
    // save image frame to local system
    if (cap.isOpened()){
        cap >> img;
        cv::imwrite(ss.str(), img);
        std::cout << ss.str() << std::endl;
    }

    // load image from local system
    std::ifstream ifs(ss.str(), std::ios::binary | std::ios::in);

    // calculate file size
    ifs.seekg(0, ifs.end);
    int length = (int)ifs.tellg();
    ifs.seekg(0, ifs.beg);
    std::cout << "filesize : " << length << std::endl;

    // read image in buffer
    char* buffer = new char [length];
    ifs.read((char*)buffer, length);
    ifs.close();

    // set format & data
    reply->set_data(std::string(buffer, length));
    
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("192.168.10.72:50051");
  transactionTestServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
