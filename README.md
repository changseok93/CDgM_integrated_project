# CDgM_integrated_project

## current state
<img src="https://github.com/changseok93/CDgM_integrated_project/blob/master/System_abstract.svg"> </img> <br>
system overview <br> <br>
<img src="https://github.com/changseok93/CDgM_integrated_project/blob/master/flowchart.svg"> </img> <br>
flow chart <br>

## requirement
optimal build and compile toolchain for local system architecture is required! (gcc, g++, cmake ....)

## how to use
### start server on firefly
1. clone repository and move in
2. move to $WORKSPACE/CDgM_integrated_project/C++/cmake/build
3. run cmake (cmake -DCMAKE_PREFIX_PATH=./local ../..)
4. run make (make -j 12)
5. run server as super user (sudo ./server)

### use client on client side
1. install grpc for python3 (python3 -m pip install grpcio)
2. install grpc protobuf code generater for python3 (python3 -m pip install grpcio-tools)
3. clone repository and move in
4. move to $WORKSPACE/CDgM_integrated_project/Python
5. autogenerate gRPC-protobuf3 code [python3 -m grpc_tools.protoc -I ./proto --python_out=. --grpc_python_out=. ./proto/my_protobuf.proto]
6. run client (python3 ./my_client.py)

## future work
1. mySQL part
2. user input customization
