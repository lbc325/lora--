var message;
var callBackList= window.top.callBackList;
var matchCode= window.top.matchCode;
var webSocket=window.top.steerWebSocket;
layui.use(['form','layer',], function(data){
    var  $ = layui.$
        ,slider = layui.slider
        ,form = layui.form
        ,layer=layui.layer;

    form.on('submit(updateTagSubmit)', function (data) {
        matchCode=(new Date()).valueOf();//获取时间戳
        var data = {"dev":1,"fun":7,"matchCode":matchCode,"raw":getFormDate()};
        message = JSON.stringify(data);
        webSocket.send(message);//发送配置舵机的数据
        callBackList.push({"matchCode":matchCode,"commandTime":matchCode+500,"callFun":"updateTag"});//将时间戳放入等待回应的列表中
        console.log("列表长度："+callBackList.length);
        //socket.send(message);
        console.log("配置舵机："+message);
        return false;
    });
});
//获取表单数据并封装
function getFormDate(){
    var EPC = parseInt(document.getElementById("EPC").value);
    var newEPC = parseInt(document.getElementById("newEPC").value);
    var raw=[{"EPC":EPC,"newEPC":EPC}];
    return raw;
}
