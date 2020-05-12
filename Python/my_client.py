from __future__ import print_function
import logging

import grpc

import my_protobuf_pb2
import my_protobuf_pb2_grpc

def run():
    with grpc.insecure_channel('localhost:50051') as channel:

        # create stub
        stub = my_protobuf_pb2_grpc.transactionTestStub(channel)
        
        # get response
        TakeImage_response = stub.IReqITORepIT(my_protobuf_pb2.RequestImageType(name='image_response_test'))
        
        # save as binary file in local system
        with open(TakeImage_response.name+'.jpg', 'wb') as file:
            file.write(TakeImage_response.data)
        
        

if __name__ == '__main__':
    run()
