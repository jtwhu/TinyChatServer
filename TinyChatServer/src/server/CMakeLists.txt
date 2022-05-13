# 将所有源文件名保存到变量SRC_LIST中
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 指定可执行文件需要链接的库文件
target_link_libraries(ChatServer muduo_net mysqlclient muduo_base hiredis pthread)

# 使用以上源文件共同参与编译，连接以上的库，生成可执行文件