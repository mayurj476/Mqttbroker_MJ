
test using :

mosquitto_pub -h 127.0.0.1 -p 1884 -t "test" -m "hello again"

mosquitto_sub -h 127.0.0.1 -p 1884 -t "test"

used mqtt protocal :
https://github.com/eclipse-mosquitto/mosquitto/blob/ff1187fd9c74ae3a7ba0097e7933828bdcdbce71/include/mqtt_protocol.h