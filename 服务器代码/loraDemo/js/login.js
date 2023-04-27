

// function check(f) {
//     var tel=f.tel.value;
//     var gcode=f.gcode.value;
//     var vcode=f.vcode.value;
//     var nickname=f.nickname.value;
//     var pwd=f.pwd.value;
//     var pwd2=f.pwd2.value;
//     if(tel.match('/^[1][3,4,5,7,8][0-9]{9}$/')&&gcode=='QNMx'&&vcode==code&&nickname.length>=6&&pwd.length>=8&&pwd.length<=12&&pwd==pwd2){
//         alert('注册成功')
//         return true;
//     }else{
//         return false;
//     }
// }



function getCode() {
    var code='';
    var set='abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789';
    for (var i=0;i<6;i++){
        var randomNum=Math.floor(Math.random()*set.length);
        code+=set[randomNum];
    }
    alert('您的验证码为:'+code);
    code='';
}



function verify1() {
    var number =document.getElementById("number");
    var password1 = document.getElementById("pwd");
    var password2 = document.getElementById("pwd2");
    var OK = document.getElementById("OK");
    var numbervl = number.value;
    var password1vl = password1.value;
    var password2vl = password2.value;
    if (password1vl === "" || password2vl === "")
    {
        alert("密码不能为空！");
        $('#myform').reset();
        // window.open("../static/login.html");
        return false;
    }
    else if (password1vl !== "1111111" || password2vl !== "1111111")
    {
        alert("输入密码有误！");
        $('#myform').reset();
        // window.open("../static/login.html");
        return false;
    }
    else if (numbervl !== "18022601218")
    {
        alert("输入账号有误！");
        $('#myform').reset();
    }
    else
    {
        alert("登陆成功！");
        // document.getElementById('result').innerText = "登录成功";
        window.close();
        // window.open("../static/readerManager.html");
        window.open("../static/readerManager.html" ,"_blank");
    }
    }