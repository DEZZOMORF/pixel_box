#pragma once

const char CAM_PAGE[] = R"HTML(<!doctype html>
<html lang="uk"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Лампа · камера</title>
<style>
body{font-family:system-ui,sans-serif;background:#111;color:#eee;max-width:480px;margin:auto;padding:16px}
button{padding:12px 14px;margin:3px;border:0;border-radius:8px;background:#333;color:#eee;font-size:16px}
button.on{background:#e8a33d;color:#111}
select{font-size:15px;padding:8px;border-radius:6px;background:#222;color:#eee;border:1px solid #444}
#pv{width:256px;height:256px;image-rendering:pixelated;background:#000;display:block;margin:12px auto;border-radius:6px}
video{width:100%;border-radius:8px;margin-top:8px}
.mut{color:#999;font-size:13px}a{color:#e8a33d}
</style></head><body>
<p><a id="back" href="#">← Керування лампою</a></p>
<h2>📷 Камера на лампу</h2>
<button id="go" class="on">Старт</button><button id="flipcam">Перемкнути камеру</button>
<label><input type="checkbox" id="mirror"> Дзеркально</label>
<p>Кадрів/с <select id="fps"><option>10</option><option selected>15</option><option>20</option><option>25</option></select></p>
<canvas id="pv" width="16" height="16"></canvas>
<div class="mut" id="st">Натисніть «Старт» і дозвольте доступ до камери.</div>
<video id="v" playsinline muted autoplay></video>
<script>
const $=id=>document.getElementById(id);
$('back').href='http://'+location.hostname+'/';
let stream=null, ws=null, timer=null, facing='environment', sent=0, t0=0;
const big=document.createElement('canvas'); big.width=big.height=64;
const bg=big.getContext('2d'), pg=$('pv').getContext('2d',{willReadFrequently:true});
bg.imageSmoothingQuality='high'; pg.imageSmoothingQuality='high';

function connect(){
  ws=new WebSocket('wss://'+location.host+'/ws'); ws.binaryType='arraybuffer';
  ws.onopen=()=>$('st').textContent='З’єднано з лампою';
  ws.onclose=()=>{ if(stream){ $('st').textContent='Перепідключення…'; setTimeout(connect,1000);} };
}
function frame(){
  const v=$('v'); if(!v.videoWidth) return;
  const s=Math.min(v.videoWidth,v.videoHeight), sx=(v.videoWidth-s)/2, sy=(v.videoHeight-s)/2;
  bg.save(); if($('mirror').checked){bg.translate(64,0); bg.scale(-1,1);}
  bg.drawImage(v,sx,sy,s,s,0,0,64,64); bg.restore();
  pg.drawImage(big,0,0,16,16);
  if(!ws||ws.readyState!==1||ws.bufferedAmount>1600) return;
  const d=pg.getImageData(0,0,16,16).data, buf=new Uint8Array(768);
  for(let i=0;i<256;i++){buf[i*3]=d[i*4];buf[i*3+1]=d[i*4+1];buf[i*3+2]=d[i*4+2];}
  ws.send(buf); sent++;
  if(performance.now()-t0>2000){ $('st').textContent='Трансляція · '+(sent/((performance.now()-t0)/1000)).toFixed(1)+' кадр/с'; sent=0; t0=performance.now(); }
}
async function start(){
  try{
    stream=await navigator.mediaDevices.getUserMedia({video:{facingMode:facing,width:{ideal:640},height:{ideal:480}},audio:false});
  }catch(e){ $('st').textContent='Немає доступу до камери: '+e.message; stream=null; return; }
  $('v').srcObject=stream; $('mirror').checked=facing==='user';
  if(!ws||ws.readyState>1) connect();
  clearInterval(timer); t0=performance.now(); sent=0;
  timer=setInterval(frame,1000/+$('fps').value);
  $('go').textContent='Стоп'; $('go').className='';
}
function stop(){
  clearInterval(timer); timer=null;
  if(stream) stream.getTracks().forEach(t=>t.stop()); stream=null;
  if(ws) ws.close(); ws=null;
  $('go').textContent='Старт'; $('go').className='on'; $('st').textContent='Зупинено. На лампі лишився останній кадр.';
}
$('go').onclick=()=>stream?stop():start();
$('flipcam').onclick=()=>{facing=facing==='user'?'environment':'user'; if(stream){stream.getTracks().forEach(t=>t.stop()); start();}};
$('fps').onchange=()=>{if(timer){clearInterval(timer); timer=setInterval(frame,1000/+$('fps').value);}};
if(!window.isSecureContext||!navigator.mediaDevices) $('st').textContent='Відкрийте цю сторінку через https://';
</script></body></html>
)HTML";
