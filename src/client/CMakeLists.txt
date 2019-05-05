#src/client
add_library(_chunker STATIC _chunker.cpp)
add_library(chunker STATIC chunker.cpp)
add_library(_encoder STATIC _encoder.cpp)
add_library(encoder STATIC encoder.cpp)
add_library(keyClient STATIC keyClient.cpp)
add_library(_sender STATIC _sender.cpp)
add_library(sender STATIC sender.cpp)
add_library(_receiver STATIC _receiver.cpp)
add_library(receiver STATIC receiver.cpp)
add_library(_decoder STATIC _decoder.cpp)
add_library(decoder STATIC decoder.cpp)
add_library(_retriever STATIC _retriever.cpp)
add_library(retriever STATIC retriever.cpp)


add_executable(client clientMain.cpp)

target_link_libraries(client ${CLIENT_OBJ} ${LINK_OBJ} hexutil)