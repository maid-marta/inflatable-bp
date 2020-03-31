#include <Stream.h>

/*
parameters
1: running
2: minPressure
3: maxPressure
4: target
5: target
6: pulse-mode enabled
7: interval
8: interval
9: denial-mode enabled
10: min sensitivity
11: max sensitivity
12: denial delta
13: denial delta
*/
const char PAGE[] PROGMEM = R"=====(
<!DOCTYPE HTML><html><head><title></title><meta http-equiv="Content-Type" content="text/html; charset=UTF-8" /></head><body>
<form action="/" method="GET">
<table>
<tr>
<td colspan="2"><input type="checkbox"
%s id="enable" /> <label
for="enable">enabled</label></td>
</tr>
<tr>
<td>pressure</td>
<td><input type="range" id="target"
min="%d" max="%d"
value="%d"
oninput="lblPressure.value = target.value" /></td>
<td><output id="lblPressure">
<span text="%d" remove="tag"></span>
</output></td>
</tr>
<tr>
<td><input type="checkbox" id="pulse" name="pulse"
%s class="radiogroup"/> <label for="pulse">pulse-mode</label>
</td>
<td><input type="range" id="interval" min="500" max="5000"
value="%d"
oninput="lblInterval.value = interval.value" /></td>
<td><output id="lblInterval">
<span text="%d" remove="tag"></span>
</output></td>
</tr>

<tr>
<td>&nbsp;</td>
</tr>

<tr>
<td><input type="checkbox" %s
id="denial"  class="radiogroup"/> <label for="denial">denial mode</label></td>
<td><input type="range" id="denialDelta"
min="%d"
max="%d"
value="%d"
oninput="lblSensitivity.value = denialDelta.value" /></td>
<td><output id="lblSensitivity">
<span text="%d" remove="tag"></span>
</output></td>
</tr>
</table>
</form>

<script inline="javascript">
var elements = [enable, denial, target, denialDelta, pulse, interval];

for(let e of elements){
e.onchange = function() {

var params="";
for(let p of elements){
if(params.length > 0){
params += "&";
}
params+=p.id+"=";

if(p.type=="checkbox"){
params+=p.checked;
}
else{
params+=p.value;
}
}

var xhttp = new XMLHttpRequest();
xhttp.open("POST", "/", true);
xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
xhttp.send(params);
}
}
</script>


</body>
</html>

)=====";
