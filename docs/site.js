const menu=document.querySelector('.menu'),nav=document.querySelector('header nav');
menu.addEventListener('click',()=>{const open=menu.getAttribute('aria-expanded')==='true';menu.setAttribute('aria-expanded',String(!open));nav.classList.toggle('open',!open)});
nav.addEventListener('click',event=>{if(event.target.closest('a')){nav.classList.remove('open');menu.setAttribute('aria-expanded','false')}});
const dialog=document.querySelector('dialog'),largeImage=dialog.querySelector('img');let dialogTrigger;
document.querySelectorAll('[data-src]').forEach(button=>button.addEventListener('click',()=>{dialogTrigger=button;const preview=button.querySelector('img');largeImage.src=preview?.currentSrc||button.dataset.src;largeImage.alt=button.dataset.alt||preview?.alt||'';dialog.showModal()}));
function closeDialog(){dialog.close()}
dialog.querySelector('.close').addEventListener('click',closeDialog);
dialog.addEventListener('click',event=>{if(event.target===dialog)closeDialog()});
dialog.addEventListener('close',()=>dialogTrigger?.focus());
document.addEventListener('keydown',event=>{if(event.key==='Escape'&&dialog.open)closeDialog()});
