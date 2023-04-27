/**
 * 读写器js
 * mod:1
 */
var readerDate = []; //标签数据
var logDate = [];//日志数据
var callBackList = new Array(); //用于存储前端调用后还未返回的命令
var readerWebSocket; //连接读写器
var matchCode; //命令匹配码
var req; //前端请求数据
var info; //此条命令的含义
var table;
$(document).ready(function() {
    readerWebSocket = TestSockets();
    readerWebSocket.onopen = onopen;
});

layui.use(['table', 'form'], function() {
    $ = layui.jquery;
    table = layui.table //表格
    var layer = layui.layer //弹层
        ,
        laypage = layui.laypage //分页
        ,


        form = layui.form,
        element = layui.element //元素操作
        //第一个表格
    table.render({
        id: 'logTableId',
        elem: '#logTable',
        limit: 20,
        toolbar: 'default', //
        data:logDate,  //表格数据
        //url: '../js/logsDate.json',//使用本地数据打开这个，注释掉上面的data行
        contentType: 'application/json',
        //page: true, //开启分页
            //,totalRow: true //开启合计行
        cols: [
                [ //表头，本地测试数据用
                    { field: "ID", align: 'center', title: 'ID' },
                    { field: "logTime", align: 'center', title: '日志时间' },
                    { field: 'logContent',  align: 'center', title: '日志内容' },
                ]
            ]
            //本地测试数据时开启
            ,
        // parseData: function(res) { //将原始数据解析成 table 组件所规定的数据
        //     return {
        //         "code": res.code, //解析接口状态
        //         // "rCode": res.rCode, //解析提示文本
        //         // "count": res.total, //解析数据长度
        //         "data": res.data //解析数据列表
        //     };
        // }
    });
    table.render({
        id: 'tagResultListId',
        elem: '#tagResultList',
        limit: 20,
        toolbar: 'default',
        data:readerDate,  //表格数据
        //url: '../js/tagsDate.json' //使用本地数据打开这个，注释掉上面的data行
        contentType: 'application/json',
        page: true, //开启分页
        //,totalRow: true //开启合计行
        cols: [
            [ //表头，本地测试数据用
                { type: 'checkbox', fixed: 'left' },
                { field: "TAGID", width: 350, align: 'center', title: 'ID' },
               // { field: "TimeStamp", width: 200, align: 'center', title: '盘点时间' },
                { field: 'RSSI', width: 180, align: 'center', title: 'RSSI(dBm)' },
                { field: 'SNR', width: 120, align: 'center', title: 'SNR' },
               // { field: 'Frequency', width: 200, align: 'center', title: 'Frequency' },
                { fixed: 'right', title: '操作', align: 'center', toolbar: '#bar' }
            ]
        ]
        //本地测试数据时开启
        // ,
        // parseData: function(res) { //将原始数据解析成 table 组件所规定的数据
        //     return {
        //         "code": res.code, //解析接口状态
        //         // "rCode": res.rCode, //解析提示文本
        //         // "count": res.total, //解析数据长度
        //         "data": res.data //解析数据列表
        //     };
        // }
    });
    //监听行工具事件
    //steerList为上面定义的表的lay-filter="steerList"
    table.on('tool(tagResultList)', function(obj) {
        var data = obj.data;
        //console.log(data)
        //点击编辑按钮
        if (obj.event === 'edit') {
            layer.open({
                title: '修改标签ID',
                type: 2,
                area: ['350px', '250px'],
                shade: 0.3,
                skin: '',
                content: 'editTag.html??id=' + data.EPC,
                success: function(layero, index) {
                    //获取当前打开的子页面id
                    var frameId = $(layero).find('iframe').attr("id");
                    console.log("frameId:" + frameId);
                    //找到子页面的document对象
                    var document = window.frames[frameId].document;
                    //根据id找到子页面中相应的元素并赋值
                    $(document).find("#EPC").val(data.EPC);
                },
                end: function() {
                    location.reload();
                }
            });
        }
    });
    readerWebSocket.onmessage = onmessage;
    //接收到消息
    function onmessage(e) {
        var jsonObject = jQuery.parseJSON(e.data); //object
        console.log("数据长度的：" + jsonObject.length)
        //将json对象转为字符串
        var formatJsonStr = JSON.stringify(jsonObject, undefined, 2);
        console.log("读写器界面收到服务端数据：" + formatJsonStr);
        //单条数据
        if(jsonObject.length == undefined){
            readerDate.push(jsonObject);
        }else{
            //多条数据
            for(var i = 0;i < jsonObject.length;i++){
                readerDate.push(jsonObject[i]);
            }
        }
        //console.log("标签数据：" + readerDate.toString());
        //表格数据重载
        table.reload('tagResultListId', {
            elem: '#tagResultList',
            data: readerDate
        });
        console.log("读取标签成功");
    }


    //提交readTagsForm表单
    // form.on('submit(readTags)', function(data) {
    //     readTags();
    /* req=getReadTagsFormDate();
     info="开始读标签";
     sendCommand(1,"readTags",info,3000,req);*/
    /* matchCode=(new Date()).valueOf();//获取时间戳
     var data = {"dev":1,"fun":5,"matchCode":matchCode,"raw":getReadTagsFormDate()};
     message = JSON.stringify(data);
     console.log("获取表单数据："+message);
     readerWebSocket.send(message);//发送数据
     callBackList.push({"dev":1,"fun":5,"matchCode":matchCode,"commandTime":matchCode+500,"callFun":"readTags"});//将时间戳放入等待回应的列表中
     console.log("列表长度："+callBackList.length);
     //socket.send(message);
     console.log("开始读标签："+message);*/
    //     return false;
    // });
    //提交configReaderForm表单
    form.on('submit(configReader)', function(data) {
        var data1 = form.val("configReaderForm");
        console.log("表单数据：" + JSON.stringify(data1));
        req = null;
        info = "配置读写器";
        sendCommand(5, "readTags", info, 500, req);
        /* matchCode=(new Date()).valueOf();//获取时间戳
         var data = {"dev":1,"fun":5,"matchCode":matchCode,"raw":getReadTagsFormDate()};
         message = JSON.stringify(data);
         console.log("获取表单数据："+message);
         readerWebSocket.send(message);//发送数据
         callBackList.push({"dev":1,"fun":5,"matchCode":matchCode,"commandTime":matchCode+500,"callFun":"readTags"});//将时间戳放入等待回应的列表中
         console.log("列表长度："+callBackList.length);
         //socket.send(message);
         console.log("配置舵机："+message);*/
        return false;
    });
    // readerWebSocket.onopen = onopen;
    // readerWebSocket.onmessage = onmessage;

    // function onmessage(e) {
    //     var count = 0;
    //     //将接收到的数据转为json对象
    //     var jsonObject = jQuery.parseJSON(e.data); //object
    //     //将json对象转为字符串
    //     var formatJsonStr = JSON.stringify(jsonObject, undefined, 2);
    //     console.log("读写器界面收到服务端数据：" + formatJsonStr);
    //     for (var i = 0; i < callBackList.length; i++) {
    //         if (callBackList[i].matchCode == jsonObject.matchCode) {
    //             count = 1;
    //             console.log("匹配成功");
    //             switch (callBackList[i].callFun) {
    //                 case "readTags":
    //                     //如果后台读标签超时，返回超时
    //                     if (jsonObject.res.hasOwnProperty('result')) {
    //                         document.getElementById("messageInfo").value = jsonObject.res.result == 'timeout' ? "读写器读标签超时！" : jsonObject.res.result;
    //                         setTimeout(function() {
    //                             document.getElementById("messageInfo").value = '';
    //                         }, 10000);
    //                         alert(JSON.stringify(jsonObject.res));
    //                         console.log("读标签结果：" + JSON.stringify(jsonObject.res.result));
    //                         break;
    //                         //读标签成功则返回数据
    //                     } else {
    //                         readerDate = jsonObject.res.data;
    //                         console.log("标签数据：" + readerDate.toString());
    //                         //表格数据重载
    //                         table.reload('tagResultListId', {
    //                             elem: '#tagResultList',
    //                             data: readerDate
    //                         });
    //                         console.log("读取标签成功");
    //                         break;
    //                     }
    //                     /*case "updateTag":
    //                         alert(JSON.stringify(jsonObject.res));
    //                         alert("修改标签EPC成功");
    //                         break;*/
    //                     /* case "configReader":
    //                          //alert(JSON.stringify(jsonObject.res));
    //                          console.log("配置读写器结果："+JSON.stringify(jsonObject.res));
    //                          break;*/
    //                     /*case "connectReader":
    //                         // alert(JSON.stringify(jsonObject.res));
    //                         console.log("连接读写器结果："+JSON.stringify(jsonObject.res));
    //                         break;*/
    //                 case "connNewReader":
    //                     alert(JSON.stringify(jsonObject.res));
    //                     console.log("连接读写器结果：" + JSON.stringify(jsonObject.res));
    //                     break;
    //                 case "setAnts":
    //                     alert(JSON.stringify(jsonObject.res));
    //                     console.log("设置天线结果：" + JSON.stringify(jsonObject.res));
    //                     break;
    //                     /*case "basicConfig":
    //                         //alert(JSON.stringify(jsonObject.res));
    //                         console.log("基本配置读写器结果："+JSON.stringify(jsonObject.res));
    //                         break;*/
    //
    //                     /*case "stopRead":
    //                         //alert(JSON.stringify(jsonObject.res));
    //                         console.log("暂停读标签结果："+JSON.stringify(jsonObject.res));
    //                         break;*/
    //                     /*case "disConnect":
    //                        // alert(JSON.stringify(jsonObject.res));
    //                         console.log("断开读写器结果："+JSON.stringify(jsonObject.res));
    //                         break;*/
    //
    //                     /*default:
    //                         alert("接收数据有误");*/
    //             }
    //             callBackList.splice(i, 1); //处理完毕删除此元素
    //             console.log("读写器列表剩余长度：" + callBackList.length)
    //         }
    //         if (count == 1) {
    //             break;
    //         }
    //
    //     }
    //
    // }
});



//1.盘点单个标签
function SingleInventory(){
    var tagId = document.getElementById("tagId").value;
    sendCommand(1,tagId,"盘点单个标签");
}
//2.盘点多个标签
function MultiInventory(){
    sendCommand(2,"","盘点多个标签");
}

//3.停止盘点
function StopInventory(){
    sendCommand(3,"","停止盘点");
}
//4.清除盘点结果
function ClearResults(){
    console.log("清除盘点结果");
    //将列表数据清空
    readerDate.length = 0;
    //表格数据重载
    table.reload('tagResultListId', {
        elem: '#tagResultList',
        data: readerDate
    });

}

//5.显示系统日志
window.onload=function (){
    var showLogBtn = document.getElementById("showLogBtn")
    showLogBtn.onclick = ShowLogs;
    function ShowLogs(){
        console.log("显示系统日志");

        var url = "../js/logsDate.json"
        var request = new XMLHttpRequest();
        //读本地json文件
        request.open("get", url);/*设置请求方法与路径*/
        request.send(null);/*不发送数据到服务器*/
        request.onload = function (){/*XHR对象获取到返回信息后执行*/
            if (request.status == 200) {/*返回状态为200，即为数据获取成功*/
                var json = JSON.parse(request.responseText);
                logDate = json.data;
                console.log(logDate);
            }
        }
        // 表格数据重载
        table.reload('logTableId', {
            elem: '#logTable',
            data: logDate
        });
    }
}

//6.清除系统日志
function ClearLogs(){
    console.log("清除系统日志");
    logDate.length = 0;
    //表格数据重载
    table.reload('logTableId', {
        elem: '#logTable',
        data: logDate
    });
}
//7.退出登录
function logout(){
    window.close();
}


function onopen(e) {
    console.log("===========reader websocket has been opened=======");
    var tip = "已进入读写器界面" //提示信息
    readerWebSocket.send(tip);
    console.log(tip);
}


//向后台发送指令
function sendCommand(fun, para,info) {
    var transData = {"fun": fun, "para": para};
    message = JSON.stringify(transData);
    readerWebSocket.send(message);
    console.log(info + ":" + message);
}

//获取readTagsForm表单数据并封装
// function getReadTagsFormDate() {
//     //取出复选框的值，选中的天线
//     var checkedAnts = [];
//     $('input[type=checkbox]:checked').each(function() {
//         /*checkedAnts.push(parseInt($(this).val()));*/
//         checkedAnts.push($(this).val());
//     });
//     console.log("天线字符串：" + checkedAnts.toString().replaceAll(",", " "))
//         /*var readTime = parseInt(document.getElementById("readTime").value);
//         var tPower=parseInt(document.getElementById("tPower").value);
//         var raw={"readpower":tPower,"readtime":readTime,"antlists":checkedAnts};*/
//         //var raw={"antlists":checkedAnts};
//     var raw = { "antlists": checkedAnts.toString().replaceAll(",", " ") };
//     return raw;
// }



//获取ConfigReaderForm表单数据并封装
// function getConfigReaderFormDate() {}

//1.开始读标签
/*function readTags(){
    req=null;
    info="读标签";
    sendCommand(1,"readTags",info,3000);
}*/

//2.连接读写器
/*function connectReader(){
    req=null;
    info="连接读写器";
    sendCommand(2,"connectReader",info,1000,req);
}*/

//6.基本配置读写器
// function basicConfig() {
//     req = getReadTagsFormDate();
//     info = "基本配置读写器";
//     sendCommand(6, "basicConfig", info, 1000, req);
// }

//4.读写器断开连接
/*function disConnect(){
    req=null;
    info = "读写器断开连接";
    sendCommand(4,"disConnect",info,500,req);
}*/
//5.连续读标签(定时发送单次读标签)
/*function conRead(){
    setInterval(function (){
        readTags();
    },5000);//每隔5s发送单次读舵机
}*/
//5.连接新读写器
// function connNewReader() {
//     req = null;
//     info = "连接读写器";
//     sendCommand(5, "connNewReader", info, 3000, req);
// }
// //6.设置天线
// function setAnts() {
//     req = getReadTagsFormDate();
//     info = "设置天线";
//     sendCommand(6, "setAnts", info, 3000, req);
// }
// //7.读标签
// function readTags() {
//     var readTime = document.getElementById("readTime").value;
//     console.log("readTime的类型：" + typeof readTime);
//     req = { "readtime": readTime };
//     info = "读标签";
//     sendCommand(7, "readTags", info, 5000, req);
// }
//修改标签EPC
/*function updateTag(){
    req=null;
    info="修改标签EPC";
    sendCommand(7,"updateTag",info,500,req);
}*/
//配置读写器
/*function configReader(){
    req=null;
    info="配置读写器";
    sendCommand(4,"configReader",info,500,req);
}*/
//暂停读标签
/*function stopRead(){
    req=null;
    info="暂停读标签";
    sendCommand(6,"stopRead",info,500,req);
}*/
//*查询待返回数据列表
// setInterval(function() {
//     isTimeOut(callBackList);
// }, 3000); //3000毫秒查询一次待返回消息列表

//*判断发送给服务器的命令是否超时
// function isTimeOut(callBackList) {
//     for (let i = 0; i < callBackList.length; i++) {
//         if (callBackList[i].commandTime < (new Date()).valueOf()) {
//             console.log(callBackList[i].callFun + "超时");
//             callBackList.splice(i, 1); //若命令超时则删除
//             console.log("读写器列表剩余长度：" + callBackList.length);
//         }
//     }
// }
// //向后台发送指令
// function sendCommand(fun, callFun, info, commandTime, req) {
//     matchCode = (new Date()).valueOf();
//     transData = { "mod": 1, "fun": fun, "matchCode": matchCode, "req": req };
//     message = JSON.stringify(transData);
//     readerWebSocket.send(message);
//     callBackList.push({ "mod": 1, "fun": fun, "matchCode": matchCode, "commandTime": matchCode + commandTime, "callFun": callFun });
//     console.log("读写器列表长度：" + callBackList.length);
//     console.log(info + ":" + message);
// }
