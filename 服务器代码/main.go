package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/gorilla/websocket"
	serial "github.com/tarm/goserial"
)

var upgrader = websocket.Upgrader{
	// 这个是校验请求来源
	// 在这里我们不做校验，直接return true
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

type MsgStruct struct {
	Fun  uint   `json:"fun"`
	Para string `json:"para"` // 请求参数
}

var channel chan []byte
var ClientSet = make(map[*websocket.Conn]int)
var s io.ReadWriteCloser

func ReadSeialThread(s io.ReadWriteCloser) {
	buf := make([]byte, 4096)
	var buffer []byte
	for {

		//n, err :=
		n, _ := s.Read(buf[:])
		/*
			if err != nil {

			} else {
				lineend := buf
			}
		*/
		//fmt.Printf(string(buf[:n]))
		buffer = append(buffer, buf[:n]...)
		//fmt.Printf(string(buffer[:]))
		if len(buffer) >= 512 {
			stringArray := strings.Split(string(buffer[:]), "\n")
			for _, v := range stringArray {
				fmt.Printf(v)
				if strings.Contains(v, "TAGID") {
					fmt.Println("匹配到：", v)
					channel <- []byte(v)

				}
				/*
					reg := regexp.MustCompile(`(TAGID)`)
					stingVec := reg.FindAllString(v, -1)
					if stingVec != nil {
						fmt.Println("匹配到：", v)
						channel <- []byte(v)
					}
				*/

			}
			buffer = nil
		}

		//fmt.Printf("%s\n", buf)

	}
}

//func ListenWebsocket() {
//
//	engine.GET("/Connect", func(context *gin.Context) {
//		fmt.Println("new client connect server==========================================================================")
//		// 将普通的http GET请求升级为websocket请求
//		client, _ := upgrader.Upgrade(context.Writer, context.Request, nil)
//		// 每隔两秒给前端推送一句消息“hello, WebSocket”
//		ClientSet[client] = 1
//		//for {
//		//	err := client.WriteMessage(websocket.TextMessage, buf)
//		//	if err != nil {
//		//		log.Println(err)
//		//		//从map中删除对应client
//		//		delete(ClientSet, client)
//		//		break
//		//	}
//		//}
//	})
//	engine.GET("/MutiInventory", func(context *gin.Context) {
//		client, _ := upgrader.Upgrade(context.Writer, context.Request, nil)
//		_, buf, _ := client.ReadMessage()
//		if string(buf[:]) == "begin" {
//			fmt.Println("向串口发送数据: AT+LORAQUERY=1")
//			_, err2 := s.Write([]byte("AT+LORAQUERY=1\r\n"))
//			if err2 != nil {
//				err := client.WriteMessage(websocket.TextMessage, []byte("Send Failed"))
//				if err != nil {
//					s.Close()
//					return
//				}
//			}
//			err := client.WriteMessage(websocket.TextMessage, []byte("Send OK"))
//			if err != nil {
//				delete(ClientSet, client)
//			}
//			ClientSet[client] = 1
//		}
//
//	})
//	engine.GET("/Close", func(context *gin.Context) {
//		client, _ := upgrader.Upgrade(context.Writer, context.Request, nil)
//		_, buf, _ := client.ReadMessage()
//		if string(buf[:]) == "begin" {
//			_, err2 := s.Write([]byte("AT+LORAQUERY=0\r\n"))
//			if err2 != nil {
//				err := client.WriteMessage(websocket.TextMessage, []byte("Send Failed"))
//				if err != nil {
//					delete(ClientSet, client)
//				}
//				s.Close()
//			}
//			err := client.WriteMessage(websocket.TextMessage, []byte("Send OK"))
//			if err != nil {
//				delete(ClientSet, client)
//			}
//			delete(ClientSet, client)
//		}
//	})
//	engine.GET("/SingleInventory", func(context *gin.Context) {
//		client, _ := upgrader.Upgrade(context.Writer, context.Request, nil)
//		_, buf, _ := client.ReadMessage()
//		if string(buf[:]) == "begin" {
//			s.Write([]byte("AT+LORAQUERY=1\r\n"))
//			ClientSet[client] = 1
//		}
//
//	})
//
//}
func msgResolve(msg []byte, conn *websocket.Conn) {
	var RecMsg MsgStruct
	err := json.Unmarshal(msg, &RecMsg)
	if err != nil {
		fmt.Println("Illegal json data received ")
		return
	}
	switch RecMsg.Fun {
	case 1:
		time.Sleep(time.Second * 5)
		fmt.Println("单个标签盘点，tagID:", RecMsg.Para)
		buf := "{\"TAGID\":\"" + RecMsg.Para + "\",\"RSSI\":-67,\"SNR\":10}"
		err := conn.WriteMessage(websocket.TextMessage, []byte(buf))
		if err != nil {
			conn.Close()
			return
		}
	case 2:
		fmt.Println("向串口发送数据: AT+LORAQUERY=1")
		s.Write([]byte("AT+LORAQUERY=1\r\n"))
		ClientSet[conn] = 1
	case 3:
		fmt.Println("向串口发送数据: AT+LORAQUERY=0")
		s.Write([]byte("AT+LORAQUERY=0\r\n"))
		delete(ClientSet, conn)

	}

}
func wsHandler(context *gin.Context) {
	/* 使用Upgrade从http请求中获取Websocket的句柄conn */
	conn, err := upgrader.Upgrade(context.Writer, context.Request, nil)
	if err != nil {
		fmt.Println("Error to connect websocket!")
		return
	}
	for {
		_, p, err := conn.ReadMessage()
		if err != nil {
			fmt.Println("[websocket]rec error")
			break
		}
		fmt.Println("[recieve]: ", string(p))
		go msgResolve(p, conn)
	}

}

func test() {
	for {
		buf := "{\"TAGID\":\"5678\",\"RSSI\":-67,\"SNR\":10}"
		channel <- []byte(buf)
		time.Sleep(time.Second * 10)
	}
}
func SendTagInfo() {
	for {
		time.Sleep(time.Microsecond * 100)
		buf := <-channel
		for conn := range ClientSet {
			err := conn.WriteMessage(websocket.TextMessage, buf[:])
			if err != nil {
				delete(ClientSet, conn)
				continue
			}
		}
	}

}

func main() {
	gin.SetMode(gin.ReleaseMode)
	gin.DisableConsoleColor()
	router := gin.New() // 创建路由
	router.Use(gin.Recovery())
	channel = make(chan []byte, 1)

	//设置串口编号
	c := &serial.Config{Name: "/dev/ttyUSB0", Baud: 115200}
	var err error
	//打开串口
	s, err = serial.OpenPort(c)
	if err != nil {
		log.Fatal(err)
		fmt.Println("打开串口失败")
	}
	fmt.Println("打开串口成功")
	//延时100
	//time.Sleep(100)

	go ReadSeialThread(s)
	//go test()
	go SendTagInfo()
	router.StaticFS("/js", http.Dir("./loraDemo/js"))
	router.StaticFS("/layui", http.Dir("./loraDemo/layui"))
	router.StaticFS("/static", http.Dir("./loraDemo/static"))
	router.StaticFile("/reader", "./loraDemo/static/readerManager.html")
	router.GET("/ws", wsHandler)
	router.Run(":8080")
	// fmt.Printf("Read %d Bytes\r\n", n)
	// fmt.Printf("%s\n", buf)

}
