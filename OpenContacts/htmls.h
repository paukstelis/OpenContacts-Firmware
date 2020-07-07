const char ap_home_html[] PROGMEM = R"(
<head>
<title>OpenContacts</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
</head>
<body>
<h3>Device MAC address: <p id='mac'></p></h3>
<style> table, th, td {	border: 0px solid black;  border-collapse: collapse;}
table#rd th { border: 1px solid black;}
table#rd td {	border: 1px solid black; border-collapse: collapse;}</style>
<caption><b>OpenContacts Device Config</caption><br><br>
<table cellspacing=4 id='rd'>
  <tr><td>SSID</td><td>Strength</td><td>Power Level</td></tr>
  <tr><td>(Scanning...)</td></tr>
</table>
<br><br>
<table cellspacing=16>
  <tr><td><input type='text' name='ssid' id='ssid' style='font-size:14pt;height:28px;'></td><td>(WiFi SSID)</td></tr>
  <tr><td><input type='password' name='pass' id='pass' style='font-size:14pt;height:28px;'></td><td>(WiFi Password)</td></tr>
  <tr><td><input type='text' name='url' id='url' style='font-size:14pt;height:28px;'></td><td><label id='lbl_auth'>OpenContacts Server URL</label></td></tr>
  <tr><td colspan=2><p id='msg'></p></td></tr>
</table>
<br>
<caption><b>OpenContacts Room Setup</caption><br>
Information for the room where this device will be installed<br>
<table cellspacing=16>
  <tr><td><input type='text' name='bldg' id='bldg' style='font-size:14pt;height:28px;'></td><td>Building</td></tr>
  <tr><td><input type='text' name='room' id='room' style='font-size:14pt;height:28px;'></td><td>Room</td></tr>
  <tr><td><input type='int' name='occup' id='occup' style='font-size:14pt;height:28px;'></td><td>Max Occupancy</td></tr>
  <tr><td><input type='checkbox' name='buzzer' id='buzzer' style='font-size:14pt;height:28px;' checked></td><td>Use Buzzer?</td></tr>
</table>
<br>
<caption><b>Admin Card Reader Setup</b></caption><br>
Check the box and complete the fields if this device will be used for assigning RFID UIDs<br>
<table cellspacing=16>
  <tr><td><input type='checkbox' name='admin_read' id='admin_read' style='font-size:14pt;height:28px;'></td><td>Admin Device?</td></tr>
  <tr><td><input type='text' name='admin_name' id='admin_name' style='font-size:14pt;height:28px;'></td><td>Device Name (if using more than one, record the name on the device)</td></tr>
  <tr><td><input type='hidden' name='admin_api' id='admin_api' style='font-size:14pt;height:28px;'></td><td></td></tr>
  <tr><td><button type='button' id='butt' onclick='sf();' style='height:36px;width:180px'>Submit</button></td><td></td></tr>
</table>
<br>
<a href="/update">Firmware Update</a><br><br>
<a href="/resetall">Reset All logs/settings</a>
</body>
<script>
function id(s) {return document.getElementById(s);}
function sel(i) {id('ssid').value=id('rd'+i).value;}
function get_check(elem) {
  var elm = document.getElementById(elem);
  if (elm.checked) {
    return 1;
  }
  return 0;
}
var tci;

//Let's set a value for api_key immediately, just for checking. This value will be store in config.
var apikey = id('admin_api');
apikey.value = makeid();

function makeid() {
  var length = 16;
  var result           = '';
  var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  var charactersLength = characters.length;
  for ( var i = 0; i < length; i++ ) {
    result += characters.charAt(Math.floor(Math.random() * charactersLength));
  }
  return result;
}

function get_mac() {
  var xhr=new XMLHttpRequest();
  xhr.onreadystatechange=function() {
    if(xhr.readyState==4 && xhr.status==200) {
      var jd=JSON.parse(xhr.responseText);
      if(!jd) { return; }
      id('mac').innerHTML = jd.mac;
      }
    };
  xhr.open('GET','gm',true); xhr.send();
}

function tryConnect() {
  var xhr=new XMLHttpRequest();
  xhr.onreadystatechange=function() {
    if(xhr.readyState==4 && xhr.status==200) {
    var jd=JSON.parse(xhr.responseText);
    if(jd.ip==0) return;
    var ip=''+(jd.ip%256)+'.'+((jd.ip/256>>0)%256)+'.'+(((jd.ip/256>>0)/256>>0)%256)+'.'+(((jd.ip/256>>0)/256>>0)/256>>0);
    id('msg').innerHTML='<b><font color=green>Connected! Device IP: '+ip+'</font></b><br>Device is rebooting. Switch back to<br>the above WiFi network, and then<br>click the button below to redirect.'
    id('butt').innerHTML='Go to '+ip;id('butt').disabled=false;
    id('butt').onclick=function rd(){window.open('http://'+ip);}
    clearInterval(tci);
    }
  }
  xhr.open('GET', 'jt', true); xhr.send();
}
function sf() {
  id('msg').innerHTML='';
  var xhr=new XMLHttpRequest();
  xhr.onreadystatechange=function() {
    if(xhr.readyState==4 && xhr.status==200) {
      var jd=JSON.parse(xhr.responseText);
      if(jd.result==1) { id('butt').innerHTML='Connecting...'; id('msg').innerHTML='<font color=gray>Connecting, please wait...</font>'; tci=setInterval(tryConnect, 2000); return; }
      id('msg').innerHTML='<b><font color=red>Error code: '+jd.result+', item: '+jd.item+'</font></b>'; id('butt').innerHTML='Submit'; id('butt').disabled=false;id('ssid').disabled=false;id('pass').disabled=false;id('auth').disabled=false;
    }
  };
  var comm='cc?ssid='+encodeURIComponent(id('ssid').value)+
         '&pass='+encodeURIComponent(id('pass').value)+
         '&url='+encodeURIComponent(id('url').value)+
         '&bldg='+encodeURIComponent(id('bldg').value)+
         '&room='+encodeURIComponent(id('room').value)+
         '&occup='+encodeURIComponent(id('occup').value)+
         '&buzzer='+get_check('buzzer')+
         '&admin_read='+get_check('admin_read')+
         '&admin_name='+encodeURIComponent(id('admin_name').value)+
         '&admin_api='+encodeURIComponent(id('admin_api').value);

           xhr.open('GET', comm, true); xhr.send();
  id('butt').disabled=true;id('ssid').disabled=true;id('pass').disabled=true;
}

function loadSSIDs() {
  var xhr=new XMLHttpRequest();
  xhr.onreadystatechange=function() {
    if(xhr.readyState==4 && xhr.status==200) {
      id('rd').deleteRow(1);
      var i, jd=JSON.parse(xhr.responseText);
      //TODO Sort and remove dups
      for(i=0;i<jd.ssids.length;i++) {
        var signalstrength= jd.rssis[i]>-71?'Ok':(jd.rssis[i]>-81?'Weak':'Poor');
        var row=id('rd').insertRow(-1);
        row.innerHTML ="<tr><td><input name='ssids' id='rd"+i+"' onclick='sel(" + i + ")' type='radio' value='"+jd.ssids[i]+"'>" + jd.ssids[i] + "</td>"  + "<td align='center'>"+signalstrength+"</td>" + "<td align='center'>("+jd.rssis[i] +" dbm)</td>" + "</tr>";
      }
    };
  }
  xhr.open('GET','js',true); xhr.send();
}
setTimeout(get_mac, 1000);
setTimeout(loadSSIDs, 1000);

</script>
</body>
)";
const char ap_update_html[] PROGMEM = R"(<head>
<title>OpenContacts Update</title>
<meta name='viewport' content='width=device-width, initial-scale=1'>
</head>
<body>
<div id='page_update'>
<div><h3>OpenContacts AP-mode Firmware Update</h3></div>
<div>
<form method='POST' action='/update' id='fm' enctype='multipart/form-data'>
<table cellspacing=4>
<tr><td><input type='file' name='file' accept='.bin' id='file'></td></tr>
<tr><td><label id='msg'></label></td></tr>
</table>
<button id='btn_submit' style='height:48px;'>Submit</a>
</form>
</div>
</div>
<script>
function id(s) {return document.getElementById(s);}
function clear_msg() {id('msg').innerHTML='';}
function show_msg(s,t,c) {
id('msg').innerHTML=s.fontcolor(c);
if(t>0) setTimeout(clear_msg, t);
}
id('btn_submit').addEventListener('click', function(e){
e.preventDefault();
var files= id('file').files;
if(files.length==0) {show_msg('Please select a file.',2000,'red'); return;}
show_msg('Uploading. Please wait...',10000,'green');
var fd = new FormData();
var file = files[0];
fd.append('file', file, file.name);
var xhr = new XMLHttpRequest();
xhr.onreadystatechange = function() {
if(xhr.readyState==4 && xhr.status==200) {
var jd=JSON.parse(xhr.responseText);
if(jd.result==1) {
show_msg('Update is successful. Rebooting. Please wait...',0,'green');
} else if (jd.result==2) {
show_msg('Check device key and try again.', 10000, 'red');
} else {
show_msg('Update failed.',0,'red');
}
}
};
xhr.open('POST', 'update', true);
xhr.send(fd);
});
</script>
</body>
)";
const char sta_home_html[] PROGMEM = R"()";
const char sta_logs_html[] PROGMEM = R"()";
const char sta_options_html[] PROGMEM = R"()";
const char sta_update_html[] PROGMEM = R"()";
