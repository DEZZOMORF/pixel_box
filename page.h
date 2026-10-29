#pragma once

const char PAGE[] = R"HTML(<!doctype html>
<html lang="uk"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Лампа</title>
<style>
:root{--bg:#111;--card:#1c1c1c;--fg:#eee;--mut:#999;--acc:#e8a33d}
*{box-sizing:border-box}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg);max-width:480px;margin:auto;padding:16px}
nav{display:flex;gap:4px}nav button{flex:1;margin:0}
button{padding:10px 12px;margin:3px;border:0;border-radius:8px;background:#333;color:var(--fg);font-size:15px;cursor:pointer}
button.on{background:var(--acc);color:#111}button.del{background:#5a2323}button:disabled{opacity:.4}
.card{background:var(--card);border-radius:12px;padding:12px;margin:12px 0}
input,select{font-size:15px;padding:8px;border-radius:6px;border:1px solid #444;background:#222;color:var(--fg);width:100%}
input[type=range]{padding:0}input[type=color]{height:44px;padding:2px}input[type=checkbox]{width:auto}
label{display:block;margin:10px 0 4px;color:var(--mut);font-size:14px}
#pv{width:192px;height:192px;image-rendering:pixelated;background:#000;display:block;margin:12px auto;border-radius:6px}
.anim{display:flex;align-items:center;gap:4px}.anim span{flex:1;overflow:hidden;text-overflow:ellipsis}
.hide{display:none}.mut{color:var(--mut);font-size:13px;margin-top:6px}
progress{width:100%;margin:8px 0}
#ed{width:100%;max-width:400px;aspect-ratio:1;display:block;margin:8px auto;touch-action:none;border-radius:6px;cursor:crosshair}
.sw{display:flex;flex-wrap:wrap;gap:4px;margin-top:6px}.sw button{width:30px;height:30px;padding:0;margin:0;border:2px solid #444}
.sw button.on{border-color:#fff}
.tools button{min-width:44px}
</style></head><body>
<nav><button data-t="ctl" class="on">Керування</button><button data-t="media">Медіа</button><button data-t="draw">Малювання</button><button data-t="set">Налаштування</button></nav>

<section id="ctl">
 <div class="card"><button id="pw">…</button></div>
 <div class="card"><label>Режим</label><div id="modes"></div>
  <div id="spdbox"><label>Швидкість анімації <span id="spdv"></span></label><input type="range" id="spd" min="-100" max="100"></div></div>
 <div class="card"><label>Яскравість</label><input type="range" id="bri" min="1" max="255">
  <label>Колір</label><input type="color" id="col"></div>
 <div class="card"><label>Фото й анімації</label><div id="anims"></div><div class="mut" id="fs"></div></div>
</section>

<section id="media" class="hide">
 <div class="card">
  <label>Фото, GIF або відео</label><input type="file" id="file" accept="image/*,video/*">
  <label>Заповнення</label>
  <select id="fit"><option value="cover">Обрізати до квадрата</option><option value="contain">Вписати повністю</option></select>
  <label>Масштабування</label>
  <select id="scale"><option value="auto">Авто</option><option value="pixel">Піксель-арт (чіткі пікселі)</option><option value="smooth">Фото (плавно)</option></select>
  <div id="vopts" class="hide">
   <label>Кадрів на секунду</label><input type="number" id="fps" value="12" min="1" max="30">
   <label>Максимум секунд</label><input type="number" id="maxs" value="10" min="1" max="120">
  </div>
  <canvas id="pv" width="16" height="16"></canvas>
  <div class="mut" id="info"></div>
  <label>Назва</label><input id="aname" maxlength="24">
  <progress id="prog" value="0" max="1" class="hide"></progress>
  <button id="send" disabled>Завантажити на лампу</button>
  <button id="toed" disabled>Відкрити в редакторі</button>
 </div>
 <div class="card"><label>Камера телефона</label>
  <button id="cam">📷 Транслювати з камери</button>
  <div class="mut">Відкриється захищена сторінка лампи (https). Браузер один раз попередить про сертифікат — натисніть «Додатково» → «Перейти». Без https браузери не дають доступу до камери.</div>
 </div>
</section>

<section id="draw" class="hide">
 <div class="card">
  <div class="tools"><button data-tool="pen" class="on" title="Олівець">✏️</button><button data-tool="erase" title="Гумка">🧽</button><button data-tool="fill" title="Заливка">🪣</button><button data-tool="pick" title="Піпетка">💧</button>
   <button id="undo" title="Скасувати">↶</button><button id="clr" title="Очистити">🗑</button></div>
  <canvas id="ed" width="321" height="321"></canvas>
  <label>Колір</label><input type="color" id="dcol" value="#ffd200">
  <div class="sw" id="pal"></div>
  <label><input type="checkbox" id="live" checked> Показувати на лампі під час малювання</label>
  <label>Відкрити збережене</label><select id="dopen"><option value="">—</option></select>
  <label>Назва</label><input id="dname" maxlength="24" placeholder="малюнок">
  <button id="dsave">Зберегти на лампу</button>
  <div class="mut" id="dinfo"></div>
 </div>
</section>

<section id="set" class="hide">
 <div class="card"><label>Орієнтація матриці</label>
  <button id="rot">Повернути на 90°</button><button id="flip">Віддзеркалити</button>
  <label><input type="checkbox" id="serp"> Матриця «змійкою»</label>
  <div class="mut">Після зміни на 5 секунд з'являється тест: верхній рядок червоний, лівий стовпчик зелений, верхній лівий кут білий.</div>
  <label>Ліміт струму, мА</label><input type="number" id="maxma" min="300" max="20000" step="100">
  <button id="saveo">Зберегти ліміт</button>
 </div>
 <div class="card">
  <label>Wi-Fi мережа</label><input id="ssid">
  <label>Пароль Wi-Fi (порожньо — не змінювати)</label><input id="wpass" type="password">
  <label>MQTT-сервер для Home Assistant (порожньо — вимкнено)</label><input id="mhost" placeholder="192.168.50.160">
  <label>MQTT порт</label><input id="mport" type="number">
  <label>MQTT логін</label><input id="muser">
  <label>MQTT пароль (порожньо — не змінювати)</label><input id="mpass" type="password">
  <button id="saven">Зберегти й перезавантажити</button>
  <div class="mut" id="netinfo"></div>
 </div>
 <div class="card"><label>Оновлення прошивки (.bin)</label><input type="file" id="fw" accept=".bin">
  <button id="fwbtn">Оновити</button><div class="mut" id="fwinfo"></div></div>
</section>

<script>
const $=id=>document.getElementById(id);
let st={}, cfgShown=false, frames=null, busy=0, pvTimer;

document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('nav button').forEach(x=>x.classList.toggle('on',x===b));
  document.querySelectorAll('section').forEach(s=>s.classList.toggle('hide',s.id!==b.dataset.t));
});

async function api(url,opt){const r=await fetch(url,opt);if(!r.ok)throw new Error(await r.text());return r.json();}
function load(j){st=j;render();}
function set(p){api('/set?'+new URLSearchParams(p)).then(load).catch(e=>alert(e.message));}
function cfgSet(p){api('/cfg?'+new URLSearchParams(p)).then(load).catch(e=>alert(e.message));}
function btn(t,f,cls){const b=document.createElement('button');b.textContent=t;b.onclick=f;if(cls)b.className=cls;return b;}

function render(){
  $('pw').className=st.power?'on':''; $('pw').textContent=st.power?'Увімкнено':'Вимкнено';
  if(document.activeElement!==$('bri')) $('bri').value=st.bri;
  $('col').value='#'+st.color.toLowerCase();
  $('spdbox').classList.toggle('hide',!st.speedFx);
  if(document.activeElement!==$('spd')){$('spd').value=fromSpd(st.speed); spdLabel();}
  $('modes').replaceChildren(...st.modes.map((m,i)=>btn(m,()=>set({mode:i}),i===st.mode?'on':'')));
  const rows=st.anims.map(a=>{
    const r=document.createElement('div'); r.className='anim';
    const s=document.createElement('span'); s.textContent=a.name+' · '+a.frames+' кадр.';
    r.append(btn('▶',()=>set({anim:a.name}),st.mode===st.animMode&&st.anim===a.name?'on':''),s,
      btn('🗑',()=>{if(confirm('Видалити «'+a.name+'»?'))api('/del?name='+encodeURIComponent(a.name)).then(load)},'del'));
    return r;});
  $('anims').replaceChildren(...rows);
  if(!rows.length) $('anims').innerHTML='<div class="mut">Ще нічого не завантажено — вкладка «Медіа»</div>';
  $('fs').textContent='Вільно '+(st.fsFree>>10)+' КБ з '+(st.fsTotal>>10)+' КБ';
  const c=st.cfg;
  $('serp').checked=c.serp;
  if(!cfgShown){cfgShown=true;$('maxma').value=c.maxma;$('ssid').value=c.ssid;$('mhost').value=c.mhost;$('mport').value=c.mport;$('muser').value=c.muser;}
  $('netinfo').textContent='IP '+st.ip+' · MQTT: '+st.mqtt;
  const sel=$('dopen'), cur=sel.value;
  sel.replaceChildren(new Option('—',''),...st.anims.map(a=>new Option(a.name+(a.frames>1?' (1-й кадр)':''),a.name)));
  sel.value=cur;
}

$('pw').onclick=()=>set({power:st.power?0:1});
$('bri').onchange=e=>set({bri:e.target.value});
$('col').onchange=e=>set({color:e.target.value.slice(1)});
const toSpd=v=>Math.abs(v)<6?100:Math.round(100*Math.pow(4,v/100));
const fromSpd=s=>Math.round(Math.log(s/100)/Math.log(4)*100);
const spdLabel=()=>$('spdv').textContent='×'+ +(toSpd(+$('spd').value)/100).toFixed(2);
$('spd').oninput=spdLabel;
$('spd').onchange=e=>set({speed:toSpd(+e.target.value)});
$('rot').onclick=()=>cfgSet({rot:(st.cfg.rot+1)%4});
$('flip').onclick=()=>cfgSet({flip:st.cfg.flip?0:1});
$('serp').onchange=e=>cfgSet({serp:e.target.checked?1:0});
$('saveo').onclick=()=>cfgSet({maxma:$('maxma').value});
$('saven').onclick=async()=>{
  if(!confirm('Зберегти й перезавантажити лампу?'))return;
  const p={};['ssid','wpass','mhost','mport','muser','mpass'].forEach(k=>p[k]=$(k).value);
  try{await fetch('/net',{method:'POST',body:new URLSearchParams(p)});}catch(e){}
  $('netinfo').textContent='Перезавантаження… Сторінка оновиться сама.';
  setTimeout(()=>location.reload(),12000);
};
$('fwbtn').onclick=()=>{
  const f=$('fw').files[0]; if(!f)return;
  const fd=new FormData(); fd.append('fw',f);
  const x=new XMLHttpRequest(); x.open('POST','/update');
  x.upload.onprogress=e=>$('fwinfo').textContent='Завантаження '+Math.round(e.loaded/e.total*100)+'%';
  x.onload=()=>{const ok=x.status===200;$('fwinfo').textContent=ok?'Оновлено, перезавантаження…':'Помилка: '+x.responseText;if(ok)setTimeout(()=>location.reload(),12000);};
  x.onerror=()=>$('fwinfo').textContent='Помилка зв’язку';
  x.send(fd);
};

const mk=(w,h=w)=>{const c=document.createElement('canvas');c.width=w;c.height=h;return c;};
const maxFrames=()=>Math.max(0,Math.floor((st.fsFree||0)*0.9/770));
const slug=n=>n.replace(/\.[^.]+$/,'').replace(/[^\p{L}\p{N}_-]+/gu,'_').replace(/^_+|_+$/g,'').slice(0,24);

function to16(src){
  const sw=src.videoWidth||src.naturalWidth||src.width, sh=src.videoHeight||src.naturalHeight||src.height;
  const cover=$('fit').value==='cover';
  const S=Math.min(512,cover?Math.min(sw,sh):Math.max(sw,sh));
  const k=S/(cover?Math.min(sw,sh):Math.max(sw,sh)), dw=sw*k, dh=sh*k;
  let c=mk(S), g=c.getContext('2d');
  g.fillStyle='#000'; g.fillRect(0,0,S,S); g.imageSmoothingQuality='high';
  g.drawImage(src,(S-dw)/2,(S-dh)/2,dw,dh);
  while(c.width>32){
    const n=Math.max(16,c.width>>1), t=mk(n), tg=t.getContext('2d');
    tg.imageSmoothingQuality='high'; tg.drawImage(c,0,0,n,n); c=t;
  }
  const f=mk(16), fg=f.getContext('2d', {willReadFrequently:true});
  fg.imageSmoothingQuality='high'; fg.drawImage(c,0,0,16,16);
  return fg.getImageData(0,0,16,16).data;
}

let how='';
function rgbaOf(src){
  const w=src.naturalWidth||src.width, h=src.naturalHeight||src.height;
  const c=mk(w,h), g=c.getContext('2d',{willReadFrequently:true}); g.drawImage(src,0,0);
  return {w,h,d:g.getImageData(0,0,w,h).data};
}
function grid({w,h,d}){
  const same=(i,j)=>Math.abs(d[i]-d[j])+Math.abs(d[i+1]-d[j+1])+Math.abs(d[i+2]-d[j+2])+Math.abs(d[i+3]-d[j+3])<48;
  const gcd=(a,b)=>{while(b)[a,b]=[b,a%b];return a;};
  const scan=(len,lines,at)=>{
    let g=0, phase=-1;
    for(let l=0;l<lines&&g!==1;l+=Math.max(1,lines>>6)){
      let start=-1;
      for(let i=1;i<len;i++) if(!same(at(l,i),at(l,i-1))){
        if(start>=0) g=gcd(g,i-start); else if(phase<0) phase=i;
        start=i;
      }
    }
    return [g||1, Math.max(0,phase)];
  };
  const [gx,px]=scan(w,h,(y,x)=>(y*w+x)*4), [gy,py]=scan(h,w,(x,y)=>(y*w+x)*4);
  const b=gcd(gx,gy)||1;
  return {b, ox:px%b, oy:py%b};
}
function colorCount({d}){
  const s=new Set(); for(let i=0;i<d.length&&s.size<=256;i+=4) s.add(d[i]<<24|d[i+1]<<16|d[i+2]<<8|d[i+3]); return s.size;
}
function isPixelArt(img){
  if($('scale').value!=='auto') return $('scale').value==='pixel';
  if(img.w*img.h>1024*1024||colorCount(img)>256) return false;
  return grid(img).b>=2||Math.max(img.w,img.h)<=64;
}
function pixelize(raw){
  const {b,ox,oy}=grid(raw[0].img);
  const nat=raw.map(({img})=>{
    const nw=Math.floor((img.w-ox)/b), nh=Math.floor((img.h-oy)/b), d=new Uint8ClampedArray(nw*nh*4);
    for(let y=0;y<nh;y++) for(let x=0;x<nw;x++){
      const s=((oy+y*b+(b>>1))*img.w+ox+x*b+(b>>1))*4, k=(y*nw+x)*4;
      d[k]=img.d[s]; d[k+1]=img.d[s+1]; d[k+2]=img.d[s+2]; d[k+3]=img.d[s+3];
    }
    return {w:nw,h:nh,d};
  });
  let x0=1e9,y0=1e9,x1=-1,y1=-1;
  for(const n of nat) for(let y=0;y<n.h;y++) for(let x=0;x<n.w;x++)
    if(n.d[(y*n.w+x)*4+3]>16){x0=Math.min(x0,x);y0=Math.min(y0,y);x1=Math.max(x1,x+1);y1=Math.max(y1,y+1);}
  if(x1<0){x0=0;y0=0;x1=nat[0].w;y1=nat[0].h;}
  const bw=x1-x0, bh=y1-y0;
  const s=(bw<=16&&bh<=16)?Math.max(1,Math.floor(16/Math.max(bw,bh)))
        :16/($('fit').value==='cover'?Math.min(bw,bh):Math.max(bw,bh));
  const offx=Math.floor((16-bw*s)/2), offy=Math.floor((16-bh*s)/2);
  how='піксель-арт '+bw+'×'+bh+(b>1?' (збільшений ×'+b+')':'');
  return nat.map((n,i)=>{
    const px=new Uint8ClampedArray(1024);
    for(let y=0;y<16;y++) for(let x=0;x<16;x++){
      const sx=x0+Math.floor((x-offx+0.5)/s), sy=y0+Math.floor((y-offy+0.5)/s), k=(y*16+x)*4;
      px[k+3]=255;
      if(sx<x0||sx>=x1||sy<y0||sy>=y1) continue;
      const q=(sy*n.w+sx)*4, a=n.d[q+3]/255;
      px[k]=n.d[q]*a; px[k+1]=n.d[q+1]*a; px[k+2]=n.d[q+2]*a;
    }
    return {px,delay:raw[i].delay};
  });
}

function loadImg(f){return new Promise((res,rej)=>{const i=new Image();
  i.onload=()=>res(i); i.onerror=()=>rej(new Error('Не вдалося відкрити зображення')); i.src=URL.createObjectURL(f);});}

function decodeGIF(buf,onFrame,limit){
  const b=new Uint8Array(buf); let p=0;
  const u16=()=>{const v=b[p]|b[p+1]<<8;p+=2;return v;};
  if(b[0]!==71||b[1]!==73||b[2]!==70) throw new Error('Це не GIF');
  p=6; const w=u16(), h=u16(), gf=b[p++]; p+=2;
  let gct=null; if(gf&0x80){const n=3<<((gf&7)+1); gct=b.subarray(p,p+n); p+=n;}
  const canvas=new Uint8ClampedArray(w*h*4);
  let delay=0, trans=-1, disposal=0, count=0;
  while(p<b.length && count<limit){
    const c=b[p++];
    if(c===0x3B) break;
    if(c===0x21){
      const label=b[p++];
      if(label===0xF9){p++; const pf=b[p++]; delay=u16()*10; const t=b[p++]; p++; trans=(pf&1)?t:-1; disposal=(pf>>2)&7;}
      else {let s; while((s=b[p++])) p+=s;}
      continue;
    }
    if(c!==0x2C) break;
    const ix=u16(), iy=u16(), iw=u16(), ih=u16(), pf=b[p++];
    let ct=gct; if(pf&0x80){const n=3<<((pf&7)+1); ct=b.subarray(p,p+n); p+=n;}
    const minCode=b[p++], parts=[]; let s, len=0;
    while((s=b[p++])){parts.push(b.subarray(p,p+s)); p+=s; len+=s;}
    const data=new Uint8Array(len); let o=0; for(const x of parts){data.set(x,o); o+=x.length;}
    const idx=lzw(data,minCode,iw*ih);
    const order=[];
    if(pf&0x40){for(const [st0,step] of [[0,8],[4,8],[2,4],[1,2]]) for(let r=st0;r<ih;r+=step) order.push(r);}
    const saved=disposal===3?canvas.slice():null;
    for(let i=0;i<ih;i++){
      const py=iy+(order.length?order[i]:i); if(py>=h) continue;
      for(let x=0;x<iw;x++){
        const ci=idx[i*iw+x], px=ix+x; if(ci===trans||px>=w||!ct) continue;
        const k=(py*w+px)*4; canvas[k]=ct[ci*3]; canvas[k+1]=ct[ci*3+1]; canvas[k+2]=ct[ci*3+2]; canvas[k+3]=255;
      }
    }
    onFrame(canvas,w,h,delay>10?delay:100); count++;
    if(disposal===2){for(let y=iy;y<Math.min(h,iy+ih);y++) canvas.fill(0,(y*w+ix)*4,(y*w+Math.min(w,ix+iw))*4);}
    else if(saved) canvas.set(saved);
    delay=0; trans=-1; disposal=0;
  }
}

function lzw(data,minCode,npix){
  const out=new Uint8Array(npix), clear=1<<minCode, eoi=clear+1;
  const prefix=new Int16Array(4096), suffix=new Uint8Array(4096), stack=new Uint8Array(4097);
  for(let i=0;i<clear;i++){prefix[i]=-1; suffix[i]=i;}
  let size=minCode+1, mask=(1<<size)-1, next=eoi+1, old=-1, first=0, o=0, bits=0, acc=0, dp=0;
  while(o<npix){
    while(bits<size){if(dp>=data.length) return out; acc|=data[dp++]<<bits; bits+=8;}
    let code=acc&mask; acc>>>=size; bits-=size;
    if(code===clear){size=minCode+1; mask=(1<<size)-1; next=eoi+1; old=-1; continue;}
    if(code===eoi) break;
    if(old===-1){out[o++]=suffix[code]; old=first=code; continue;}
    const cur=code; let sp=0;
    if(code>=next){stack[sp++]=first; code=old;}
    while(code>=clear){stack[sp++]=suffix[code]; code=prefix[code];}
    first=suffix[code]; stack[sp++]=first;
    while(sp>0&&o<npix) out[o++]=stack[--sp];
    if(next<4096){prefix[next]=old; suffix[next]=first; next++; if(next>mask&&size<12){size++; mask=(1<<size)-1;}}
    old=cur;
  }
  return out;
}

async function gifFrames(f){
  const buf=await f.arrayBuffer();
  let first=null;
  decodeGIF(buf,(rgba,w,h)=>{first={w,h,d:new Uint8ClampedArray(rgba)};},1);
  if(first&&first.w*first.h<=512*512&&isPixelArt(first)){
    const raw=[];
    decodeGIF(buf,(rgba,w,h,delay)=>raw.push({img:{w,h,d:new Uint8ClampedArray(rgba)},delay}),maxFrames());
    return pixelize(raw);
  }
  const out=[]; let c=null;
  decodeGIF(buf,(rgba,w,h,delay)=>{
    if(!c) c=mk(w,h);
    c.getContext('2d').putImageData(new ImageData(new Uint8ClampedArray(rgba),w,h),0,0);
    out.push({px:to16(c),delay});
  },maxFrames());
  return out;
}

async function videoFrames(f,my){
  const v=document.createElement('video');
  v.muted=true; v.playsInline=true; v.preload='auto'; v.src=URL.createObjectURL(f);
  await new Promise((res,rej)=>{v.onloadeddata=res; v.onerror=()=>rej(new Error('Браузер не може відкрити це відео'));});
  const fps=Math.min(30,Math.max(1,+$('fps').value||12)), maxs=+$('maxs').value||10;
  const dur=isFinite(v.duration)?Math.min(v.duration,maxs):maxs;
  const n=Math.min(maxFrames(),Math.max(1,Math.floor(dur*fps))), out=[];
  for(let i=0;i<n;i++){
    if(my!==busy) return null;
    await new Promise(r=>{v.onseeked=r; v.currentTime=(i+0.5)/fps;});
    out.push({px:to16(v),delay:Math.round(1000/fps)});
    $('info').textContent='Обробка відео… '+(i+1)+'/'+n;
  }
  URL.revokeObjectURL(v.src);
  return out;
}

async function convert(){
  const f=$('file').files[0]; if(!f) return;
  const my=++busy; frames=null; clearTimeout(pvTimer); $('send').disabled=true; $('toed').disabled=true;
  $('vopts').classList.toggle('hide',!f.type.startsWith('video/'));
  if(maxFrames()<1){$('info').textContent='На лампі немає місця — видаліть щось у «Керуванні»'; return;}
  $('info').textContent='Обробка…';
  try{
    let out; how='';
    if(f.type==='image/gif') out=await gifFrames(f);
    else if(f.type.startsWith('video/')) out=await videoFrames(f,my);
    else {
      const img=await loadImg(f), w=img.naturalWidth, h=img.naturalHeight;
      const raw=w*h<=4096*4096?rgbaOf(img):null;
      out=raw&&isPixelArt(raw)?pixelize([{img:raw,delay:0}]):[{px:to16(img),delay:0}];
    }
    if(my!==busy||!out) return;
    if(!out.length) throw new Error('Не вдалося отримати жодного кадру');
    frames=out;
    const kb=Math.ceil((6+out.length*770)/1024), cut=out.length>=maxFrames()?' (обрізано — більше не влізе)':'';
    $('info').textContent=out.length+' кадр., '+kb+' КБ'+cut+' · '+(how||'плавне масштабування');
    $('send').disabled=false; $('toed').disabled=false;
    preview();
  }catch(e){if(my===busy)$('info').textContent='Помилка: '+e.message;}
}

function preview(){
  clearTimeout(pvTimer); if(!frames) return;
  const g=$('pv').getContext('2d'); let i=0;
  const step=()=>{const fr=frames[i]; g.putImageData(new ImageData(new Uint8ClampedArray(fr.px),16,16),0,0);
    i=(i+1)%frames.length; if(frames.length>1) pvTimer=setTimeout(step,fr.delay||100);};
  step();
}

$('file').onchange=()=>{const f=$('file').files[0]; if(f){$('aname').value=slug(f.name)||('anim'+Date.now()%100000); convert();}};
['fit','scale','fps','maxs'].forEach(id=>$(id).onchange=convert);

function packFrames(frames){
  const n=frames.length, buf=new Uint8Array(6+n*770);
  buf.set([76,49,54,65]); buf[4]=n&255; buf[5]=n>>8;
  let o=6;
  for(const fr of frames){
    const d=Math.min(65535,fr.delay|0); buf[o++]=d&255; buf[o++]=d>>8;
    for(let i=0;i<256;i++){buf[o++]=fr.px[i*4]; buf[o++]=fr.px[i*4+1]; buf[o++]=fr.px[i*4+2];}
  }
  return buf;
}
$('send').onclick=()=>{
  const name=slug($('aname').value); if(!name){alert('Вкажіть назву');return;}
  const buf=packFrames(frames);
  const fd=new FormData(); fd.append('f',new Blob([buf]),'anim.bin');
  const x=new XMLHttpRequest(); x.open('POST','/upload?name='+encodeURIComponent(name));
  $('prog').classList.remove('hide'); $('send').disabled=true;
  x.upload.onprogress=e=>$('prog').value=e.loaded/e.total;
  x.onload=()=>{$('prog').classList.add('hide'); $('send').disabled=false;
    if(x.status===200){load(JSON.parse(x.responseText)); $('info').textContent='Готово, грає на лампі ✓';}
    else alert(x.responseText);};
  x.onerror=()=>{$('prog').classList.add('hide'); $('send').disabled=false; alert('Помилка зв’язку з лампою');};
  x.send(fd);
};

const PAL=['#000000','#ffffff','#ff0000','#ff8000','#ffd200','#00ff00','#00ffff','#0000ff','#8000ff','#ff00ff','#ff6090','#804000','#808080','#204020','#003080','#ffb080'];
let pix=new Uint8ClampedArray(1024), tool='pen', hist=[], drawing=false, liveTimer=null, liveDirty=false;
for(let i=3;i<1024;i+=4) pix[i]=255;
const hex2rgb=h=>[1,3,5].map(i=>parseInt(h.substr(i,2),16));
function edRender(){
  const g=$('ed').getContext('2d'); g.fillStyle='#333'; g.fillRect(0,0,321,321);
  for(let i=0;i<256;i++){const k=i*4; g.fillStyle='rgb('+pix[k]+','+pix[k+1]+','+pix[k+2]+')'; g.fillRect((i%16)*20+1,(i>>4)*20+1,19,19);}
}
function pushLive(){
  if(!$('live').checked) return;
  liveDirty=true; if(liveTimer) return;
  const go=()=>{ if(!liveDirty){liveTimer=null;return;} liveDirty=false;
    let h=''; for(let i=0;i<256;i++) for(let c=0;c<3;c++) h+=pix[i*4+c].toString(16).padStart(2,'0');
    fetch('/draw',{method:'POST',body:new URLSearchParams({px:h})}).catch(()=>{}).finally(()=>liveTimer=setTimeout(go,60)); };
  go();
}
function cellAt(e){const r=$('ed').getBoundingClientRect(); return [Math.floor((e.clientX-r.left)/r.width*16),Math.floor((e.clientY-r.top)/r.height*16)];}
function setPx(x,y,rgb){if(x<0||y<0||x>15||y>15)return; const k=(y*16+x)*4; pix[k]=rgb[0];pix[k+1]=rgb[1];pix[k+2]=rgb[2];}
function getPx(x,y){const k=(y*16+x)*4; return [pix[k],pix[k+1],pix[k+2]];}
function flood(x,y,rgb){
  const t=getPx(x,y).join(); if(t===rgb.join()) return;
  const q=[[x,y]]; while(q.length){const [a,b]=q.pop(); if(a<0||b<0||a>15||b>15||getPx(a,b).join()!==t) continue;
    setPx(a,b,rgb); q.push([a+1,b],[a-1,b],[a,b+1],[a,b-1]);}
}
function apply(e,first){
  const [x,y]=cellAt(e), rgb=hex2rgb($('dcol').value);
  if(tool==='pick'){ if(first){$('dcol').value='#'+getPx(x,y).map(v=>v.toString(16).padStart(2,'0')).join(''); renderPal();} return; }
  if(tool==='fill'){ if(first) flood(x,y,rgb); }
  else setPx(x,y,tool==='erase'?[0,0,0]:rgb);
  edRender(); pushLive();
}
function snapshot(){hist.push(pix.slice()); if(hist.length>50) hist.shift();}
$('ed').onpointerdown=e=>{e.preventDefault(); $('ed').setPointerCapture(e.pointerId); if(tool!=='pick') snapshot(); drawing=true; apply(e,true);};
$('ed').onpointermove=e=>{if(drawing&&(tool==='pen'||tool==='erase')) apply(e,false);};
$('ed').onpointerup=$('ed').onpointercancel=()=>drawing=false;
document.querySelectorAll('[data-tool]').forEach(b=>b.onclick=()=>{tool=b.dataset.tool;
  document.querySelectorAll('[data-tool]').forEach(x=>x.classList.toggle('on',x===b));});
$('undo').onclick=()=>{if(hist.length){pix=hist.pop(); edRender(); pushLive();}};
$('clr').onclick=()=>{snapshot(); for(let i=0;i<1024;i+=4){pix[i]=pix[i+1]=pix[i+2]=0;} edRender(); pushLive();};
function renderPal(){
  $('pal').replaceChildren(...PAL.map(c=>{const b=document.createElement('button'); b.style.background=c;
    if(c===$('dcol').value) b.className='on'; b.onclick=()=>{$('dcol').value=c; if(tool==='erase'||tool==='pick') document.querySelector('[data-tool=pen]').click(); renderPal();}; return b;}));
}
$('dcol').oninput=renderPal;
function loadPix(px,name){snapshot(); pix=new Uint8ClampedArray(px); edRender(); pushLive(); if(name) $('dname').value=name;
  document.querySelector('nav button[data-t=draw]').click();}
$('toed').onclick=()=>{if(frames) loadPix(frames[0].px,$('aname').value);};
$('dopen').onchange=async()=>{
  const n=$('dopen').value; if(!n) return;
  const b=new Uint8Array(await (await fetch('/file?name='+encodeURIComponent(n))).arrayBuffer());
  const px=new Uint8ClampedArray(1024); for(let i=0;i<256;i++){px[i*4]=b[8+i*3];px[i*4+1]=b[9+i*3];px[i*4+2]=b[10+i*3];px[i*4+3]=255;}
  loadPix(px,n);
};
$('dsave').onclick=()=>{
  const name=slug($('dname').value)||('pic'+Date.now()%100000);
  const fd=new FormData(); fd.append('f',new Blob([packFrames([{px:pix,delay:0}])]),'pic.bin');
  fetch('/upload?name='+encodeURIComponent(name),{method:'POST',body:fd})
    .then(async r=>{if(!r.ok) throw new Error(await r.text()); load(await r.json()); $('dinfo').textContent='Збережено як «'+name+'» ✓';})
    .catch(e=>alert(e.message));
};
edRender(); renderPal();
$('cam').onclick=()=>location.href='https://'+location.hostname+'/';

api('/state').then(load);
setInterval(()=>{if(!document.hidden&&!$('ctl').classList.contains('hide'))api('/state').then(load).catch(()=>{});},5000);
</script></body></html>
)HTML";
