#include "mbed.h"
#include "Sht31.h"
#include "MQTTmbed.h"
#include "MQTTClientMbedOs.h"
#include <cstdio>


Sht31 temp_sensor(D14, D15);
DigitalOut rele(A0);
WiFiInterface *wifi;
int arrivedcount = 0;


void messageArrived(MQTT::MessageData& md)
{

    ++arrivedcount;
}

void mqtt_demo(NetworkInterface *net)
{
    char* topic = "temp";
    TCPSocket socket;
    MQTTClient client(&socket);
    SocketAddress a;
    char* hostname = "dev.rightech.io";
    
    net->gethostbyname(hostname, &a);
    int port = 1883;
    a.set_port(port);
    
    socket.open(net);
    int rc = socket.connect(a);
    if (rc != 0) {
        printf("[DEBUG] TCP connect failed: %d\r\n", rc);
        return;
    }
    
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "nasibullinasalima-5iompi";
    data.username.cstring = "123";
    data.password.cstring = "123";
    data.keepAliveInterval = 60;

    printf("[DEBUG] Connecting to MQTT...\r\n");
    
    rc = client.connect(data);
    if (rc != 0) {
        printf("[DEBUG] MQTT connect failed with code %d\r\n", rc);
        socket.close();
        return; 
    }
    printf("[DEBUG] MQTT Connected successfully!\r\n");
    
    if ((rc = client.subscribe(topic, MQTT::QOS0, messageArrived)) != 0)
        printf("[DEBUG] Subscribe error: %d\r\n", rc);
    
    MQTT::Message message;
    char buf[100];
    float t;

    while (true) {
        t = temp_sensor.readTemperature();
        

        int t_int = round(t); 
        sprintf(buf, "%d", t_int);
        
        printf("[DEBUG] Sending temp: %d C\r\n", t_int);
        rele=1;
        if (t_int > 30) {
            rele=!rele ;
            printf("[DEBUG] FIRE");
        };
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = (void*)buf;
        message.payloadlen = strlen(buf); 
        
        rc = client.publish(topic, message);
        if (rc != 0) {
            printf("[DEBUG] Publish error: %d. Connection lost.\r\n", rc);
            break; 
        }
        
        client.yield(200); 
        wait_ms(300); 
    }
}

const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE: return "None";
        case NSAPI_SECURITY_WEP: return "WEP";
        case NSAPI_SECURITY_WPA: return "WPA";
        case NSAPI_SECURITY_WPA2: return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2: return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default: return "Unknown";
    }
}

int scan_demo(WiFiInterface *wifi)
{
    WiFiAccessPoint *ap;
    printf("Scan:\r\n");
    int count = wifi->scan(NULL,0);
    if (count <= 0) return 0;
    
    count = count < 15 ? count : 15;
    ap = new WiFiAccessPoint[count];
    count = wifi->scan(ap, count);
    if (count <= 0) return 0;
    
    for (int i = 0; i < count; i++) {
        printf("Network: %s secured: %s\r\n", ap[i].get_ssid(), sec2str(ap[i].get_security()));
    }
    delete[] ap;
    return count;
}

int main()
{
    
    float t;
    t = temp_sensor.readTemperature();
    
    int t_int = round(t); 
    
        
    printf("[DEBUG] Sending temp: %d C\r\n", t_int);
    printf("[DEBUG] Program started\r\n");
    
    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\r\n");
        return -1;
    }
    
    scan_demo(wifi);
    
    printf("Connecting to WiFi...\r\n");
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("WiFi Connection error: %d\r\n", ret);
        return -1;
    }
    
    printf("WiFi Success! IP: %s\r\n", wifi->get_ip_address());
    
    mqtt_demo(wifi);
    
    wifi->disconnect();
    printf("Done\r\n");
    return 0;
}
