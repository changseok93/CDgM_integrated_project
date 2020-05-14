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
#include </usr/include/mysql/mysql.h>

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
using namespace std;

// Logic and data behind the server's behavior.
class transactionTestServiceImpl final : public transactionTest::Service {
public:
  MYSQL *conn;
  MYSQL_RES *res;
  MYSQL_ROW row;
  fstream pin12;
  ifstream pin39;
  ifstream pin42;

  transactionTestServiceImpl(){

    // initialize Mysql Component
    char *server = "192.168.10.69";
    char *user = "root";
    char *password = "return123";
    char *database = "test";

    if( !(conn = mysql_init((MYSQL*)NULL))){
            printf("init fail\n");
            exit(1);
    }

    printf("mysql_init success.\n");

    if(!mysql_real_connect(conn, server, user, password, NULL, 3306, NULL, 0)){
            printf("connect error.\n");
            exit(1);
    }

    printf("mysql_real_connect success\n");

    if(mysql_select_db(conn, database) != 0){
            mysql_close(conn);
            printf("select_db fail.\n");
            exit(1);
    }
    printf("select mydb success\n");

    // initialize GPIO component
    pin12.open("/sys/class/gpio/gpio12/value");
    pin39.open("/sys/class/gpio/gpio39/value");
    pin42.open("/sys/class/gpio/gpio42/value");
    printf("pin fd registration success\n");

  }

  Status IReqITORepIT(ServerContext* context, const RequestImageType* request, ReplyImageType* reply) override {    
    //set local image path
    std::stringstream ss;
    ss << "../../image_log/" << request->name() << ".jpg"; 

    // door open
    printf("[sys] unlock\n");
    pin12 << "0";
    pin12.seekg(0);
    usleep(1500000);

    char lock, door;
    // wait until door close
    while(true){
      pin42.seekg(0);
      pin39.seekg(0);
      pin42 >> lock;
      pin39 >> door;

      if (lock == '1' && door == '0'){
        printf("[sys] door closed\n");
        break;
      }
      usleep(100000);
    }

    // get image frame from camera
    cv::Mat img;
    cv::VideoCapture cap;
    cap.open("/dev/video10");
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 0x7FFFFFFF);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 0x7FFFFFFF);
    cap >> img;
    printf("[sys] get image frame, size : %d\n", img.total());

    //jpeg compression parameters
    std::vector<uchar> buff;
    std::vector<int> param = std::vector<int>(2);
    param[0]=cv::IMWRITE_JPEG_QUALITY;
    param[1]=95;

    //jpeg compression
    imencode(".jpg",img,buff,param);
    const char* data = reinterpret_cast<char*>(buff.data());
    int length = buff.size();
    printf("[sys] JPEG encoding, size : %d\n", length);


    
    // -----------------------------------------------------------------------------------------------
    char *query,*end;
    query = new char [2*length + 1000];

    end = stpcpy(query,"INSERT INTO Image (env_id, data, type, check_num) VALUES('2', '");
    end += mysql_real_escape_string(conn,end,data,length);
    end = stpcpy(end,"','1', '1");
    end = stpcpy(end,"')");

    if (mysql_real_query(conn,query,(unsigned int) (end - query)))
    {
       fprintf(stderr, "Failed to insert row, Error: %s\n",
               mysql_error(conn));
    }
    printf("[sys] save image in mysql_server success\n");
    // -----------------------------------------------------------------------------------------------

    // set reply form
    reply->set_data(std::string(data, length));
    reply->set_name(request->name());
    printf("[sys] create gRPC reply form success\n");

    delete query;
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("192.168.0.17:50051");
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

