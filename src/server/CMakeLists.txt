# 选择目录下所有服务器源文件至SRC_LIST中
aux_source_directory(. SRC_LIST)
# 选择db目录下MYSQL源文件至DB_LIST中
aux_source_directory(./db DB_LIST)
# 选择model目录下Model源文件至MODEL_LIST
aux_source_directory(./model MODEL_LIST)
# 选择redis目录下Reids源文件至REDIS_LIST
aux_source_directory(./redis REDIS_LIST)
# 选择kafka目录下Kafka源文件至KAFKA_LIST
aux_source_directory(./kafka KAFKA_LIST)

# 指定生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST} ${KAFKA_LIST})

# 链接动态库
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis rdkafka++ pthread)