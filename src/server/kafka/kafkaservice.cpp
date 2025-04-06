#include "kafkaservice.hpp"

KafkaService::KafkaService() : _updating(false), _running(false) {}

KafkaService::~KafkaService()
{
    close();
}

bool KafkaService::connect()
{
    string broker = "localhost:9092";
    string errstr;

    Conf *conf = Conf::create(Conf::CONF_GLOBAL);
    conf->set("bootstrap.servers", broker, errstr);

    // 创建生产者
    _producer = Producer::create(conf, errstr);
    if (!_producer)
    {
        cerr << "Create kafka producer failed!" << endl;
        cerr << errstr << endl;
        return false;
    }

    // 创建消费者
    conf->set("group.id", "consumer-client", errstr);
    conf->set("auto.offset.reset", "earliest", errstr);

    _consumer = KafkaConsumer::create(conf, errstr);
    if (!_consumer)
    {
        cerr << "Create kafka consumer failed!" << endl;
        cerr << errstr << endl;
        return false;
    }

    // 开启从消息队列中获取消息的线程
    _running = true;
    consume_thread = thread(&KafkaService::consume_loop, this);

    return true;
}

bool KafkaService::publish(string &topic, string &msg)
{
    ErrorCode resp = _producer->produce(topic, Topic::PARTITION_UA, Producer::RK_MSG_COPY,
                                        const_cast<char *>(msg.c_str()), msg.size(),
                                        nullptr, 0, 0, nullptr);
    if (resp != ERR_NO_ERROR)
    {
        cerr << "Send message failed" << endl;
        cerr << err2str(resp) << endl;
        return false;
    }

    _producer->poll(0);
    _producer->flush(3000);

    cout << "Publish successful" << endl;

    return true;
}

void KafkaService::subscribe(string &topic)
{
    cout << "subscribing " << topic << endl;

    {
        lock_guard<mutex> g1(mtx);
        // 向topics中添加topic
        _topics.insert(topic);
    }

    // 更新消费者的订阅主题
    update();
}

void KafkaService::unsubscribe(string &topic)
{
    lock_guard<mutex> g1(mtx);

    // 向topics中删除topic
    _topics.erase(topic);

    // 更新消费者的订阅主题
    update();
}

void KafkaService::update()
{
    _updating = true;

    if (_topics.empty())
    {
        _consumer->unsubscribe();
        _updating = false;
        return;
    }

    vector<string> topic_set(_topics.begin(), _topics.end());

    ErrorCode resp = _consumer->subscribe(topic_set);
    if (resp != ERR_NO_ERROR)
    {
        cerr << "Subscribe failed!" << endl;
        cerr << err2str(resp) << endl;
    }

    cout << "Now subscribed to topics: ";
    for (auto &t : topic_set)
        cout << t << " ";
    cout << endl;

    _updating = false;
}

void KafkaService::init_notify_handler(function<void(string, string)> fn)
{
    _notify_handler = fn;
}

void KafkaService::consume_loop()
{
    while (_running)
    {
        // Topic更新时进行空转
        if (!_updating)
        {
            Message *msg = _consumer->consume(1000);
            if (msg->err() == ERR_NO_ERROR)
            {
                cout << "Getcha" << endl;
                if (msg)
                {
                    cout << "it has topic" << endl;
                    cout << msg->topic_name() << endl;
                    if (msg->payload())
                    {
                        cout << "it has payload" << endl;
                        cout << string(static_cast<char *>(msg->payload()), msg->len()) << endl;

                        cout << "Not null" << endl;

                        // 捕获到来自其他服务器的消息
                        _notify_handler(msg->topic_name(), string(static_cast<char *>(msg->payload()), msg->len()));
                    }
                }
            }
            else if (msg->err() == ERR__TIMED_OUT)
            {
                // 消息队列为空
                continue;
            }
            else
            {
                cerr << err2str(msg->err()) << endl;
            }
        }
    }
}

void KafkaService::close()
{
    _running = false;

    if (_consumer)
    {
        _consumer->close();
    }

    if (consume_thread.joinable())
    {
        consume_thread.join();
    }
}