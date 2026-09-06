const fs=require('fs'),vm=require('vm'),assert=require('assert');
const code=fs.readFileSync('docs/locale.js','utf8');
const locales=JSON.parse(fs.readFileSync('localization/locales.json'));
function run(url,lang='en',stored=null,primary='en-US',blockedStorage=false,hrefs=[]) {
 let destination=null,handler;
 const location=new URL(url); location.replace=location.assign=u=>destination=u;
 const picker={options:locales.map(l=>({value:l.code})),addEventListener:(e,h)=>handler=h};
 const body={dataset:{siteRoot:lang==='en'?'':'../',page:url.includes('changelog')?'changelog':'home'}};
 const localStorage={getItem:()=>{if(blockedStorage)throw Error('Storage blocked');return stored},setItem:(k,v)=>{if(blockedStorage)throw Error('Storage blocked');stored=v}};
 const links=hrefs.map(href=>({getAttribute:()=>href,setAttribute:(key,value)=>href=value}));
 vm.runInNewContext(code,{document:{querySelector:()=>picker,querySelectorAll:()=>links,documentElement:{lang},body},window:{location},localStorage,history:{replaceState:(a,b,u)=>destination=String(u)},navigator:{languages:[primary,'fr']},URL,URLSearchParams,Intl});
 return {destination,stored,picker,links,choose:l=>{picker.value=l;handler();return destination}};
}
const base='http://localhost:8768/';
for (const [q,expected] of [['pt-Latn-PT','pt-PT'],['pt-PT-u-nu-latn','pt-PT'],['ar-u-nu-latn','ar'],['pt','pt-BR'],['pt-PT','pt-PT'],['pt-BR','pt-BR'],['no-NO','nb'],['nb-NO','nb'],['zh-Hans-TW','zh-Hans'],['zh-Hant-CN','zh-Hant'],['iw','he'],['tl','fil']]) assert.equal(new URL(run(base+'?lang='+q).destination).pathname,'/'+expected+'/');
assert.equal(run(base,'en',null,'xx-XX').destination,null);
assert.equal(run(base+'fr/','fr','de','de').destination,null);
assert.equal(new URL(run(base+'changelog.html?lang=ar&platform=windows#macos-changelog').destination).search,'?platform=windows');
assert.equal(run(base,'en','fr').destination,base+'fr/');
assert.equal(run(base+'ar/','ar').choose('en'),base);
assert.equal(run(base+'?lang=en','en','fr').stored,'en');
assert.equal(run(base+'?lang=sr-Latn').destination,base);
console.log('PASS: locale query aliases, script preference, primary-only matching, saved preference, explicit routes, canonical URLs, query/hash preservation');

assert.equal(run(base+'?lang=zh-Latn').destination,base);

// Explicit English remains selectable even if browser storage is unavailable.
const englishChoice = run(base+'fr/', 'fr', null, 'fr-FR', true).choose('en');
assert.equal(englishChoice, base+'?lang=en');
const englishLoad = run(englishChoice, 'en', null, 'fr-FR', true, ['changelog.html?platform=windows', '#features', 'https://github.com/kiteretsu903/eink-assistant', 'fr/']);
assert.equal(englishLoad.destination, base+'?lang=en');
assert.equal(englishLoad.picker.value, 'en');
assert.equal(englishLoad.links[0].getAttribute('href'), '/changelog.html?platform=windows&lang=en');
assert.equal(englishLoad.links[1].getAttribute('href'), '/?lang=en#features');
assert.equal(englishLoad.links[2].getAttribute('href'), 'https://github.com/kiteretsu903/eink-assistant');
assert.equal(englishLoad.links[3].getAttribute('href'), 'fr/');
assert.equal(run(englishLoad.destination, 'en', null, 'fr-FR', true).destination, base+'?lang=en');
assert.equal(run(base+'index.html?lang=en#features', 'en', null, 'fr-FR', true).destination, base+'?lang=en#features');
assert.equal(run(base+'changelog.html?lang=en&platform=windows', 'en', null, 'fr-FR', true).destination, base+'changelog.html?lang=en&platform=windows');
assert.equal(run(base+'fr/', 'fr', null, 'fr-FR', true).choose('de'), base+'de/');
// Project-site paths are retained when carrying English intent across links.
const project = 'https://example.github.io/eink-assistant/';
const projectLoad = run(project+'?lang=en', 'en', null, 'fr-FR', true, ['./changelog.html', '#features']);
assert.equal(projectLoad.links[0].getAttribute('href'), '/eink-assistant/changelog.html?lang=en');
assert.equal(projectLoad.links[1].getAttribute('href'), '/eink-assistant/?lang=en#features');
console.log('PASS: blocked-storage English selection, reload, internal links, query/hash and project-path preservation');
