# cpp-gossip
C++ implementation of gossip protocol
This project is based on [memberlist](https://github.com/hashicorp/memberlist) implemented by golang.


## Serialization and Deserialization
This project uses [google protobuf v3.19.4](https://github.com/protocolbuffers/protobuf) to serialize and deserialize the structural data.
[glog v0.6.0](https://github.com/google/glog)
## Time Accuracy
The time accuracy is microsecond level.