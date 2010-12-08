changequote({{,}})dnl
ifdef({{conf_GSERVICE_SUPPORT}}, {{}}, {{m4exit(1)}})dnl
ifdef({{conf_GSERVICE_SUPPORT_INLINE}}, {{}}, {{m4exit(1)}})dnl
<html>
<head>
<title>Google Services</title>
<link rel="stylesheet" href="Sty.c" type="text/css"/>
<script src="scr.js" type="text/javascript"></script>
<script type="text/javascript">
function fillFields() {
ifdef({{conf_GWEATHER_SUPPORT}}, {{}}, {{dnl
	getCmd('weather city', writeVal, $('weather_city'));
}})dnl
ifdef({{conf_GCALENDAR_SUPPORT}}, {{dnl
	getCmd('calendar login', writeVal, $('calendar_login'));
}})dnl

}

function getCmd(cmd, handler, data) {
	ArrAjax.ecmd(cmd, handler, 'GET', data);
}
	
function setCmd(cmd, value) {
	ArrAjax.ecmd(cmd + ' ' + value);
}

function writeVal(request, data) {
	data.value = request.responseText;
}
	
function changeState(request, data) {
	data.style.backgroundColor = (request.responseText == "OK\n") ? "green" : "red";
}

</script>
</head><body onLoad='fillFields()'>
<h1>Ethersex Setup</h1>
<div id="valdiv">
<center><table>
ifdef({{conf_GWEATHER_SUPPORT}}, {{}}, {{dnl
	<tr>
	<td>Google weather</td>
	<td><input type="text" id="weather_city" onChange='getCmd("weather_city " + this.value, changeState, this);'></td>
	</tr>
}})dnl
ifdef({{conf_GCALENDAR_SUPPORT}},{{dnl
	<tr>
	<td>Google calendar</td>
	<td><input type="text" id="calendar_login" onChange='getCmd("calendar_login" + this.value, changeState, this);'></td>
	</tr>
}})dnl
</table></center>
<a href="idx.ht"> Back </a>
</div>
<div id="logconsole"></div>
</body>
</html>
