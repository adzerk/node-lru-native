const LRUCache = require('../index.js');
const Cache = new LRUCache();

Cache.set('test', 1);
console.log('Set/Get');
console.log(Cache.get('test'));
console.log('\n\n');

console.log('SetMaxElements: remain 5 items');
for (var i=0; i < 10; i++) {
	Cache.set('test'+i, i);
}
Cache.setMaxElements(5);
for (var i=0; i < 10; i++) {
	console.log(i, Cache.get('test'+i));
}
console.log('\n\n');

console.log('SetMaxAge #1: remain 5 items');
Cache.setMaxAge(1000);
for (var i=0; i < 10; i++) {
	console.log(i, Cache.get('test'+i));
}
console.log('\n\n');

setTimeout(function() {
		   console.log('SetMaxAge #2: remain 5 items');
		   for (var i=0; i < 10; i++) {
		   console.log(i, Cache.get('test'+i));
		   }
		   console.log('\n\n');
		   }, 500);

setTimeout(function() {
		   console.log('SetMaxAge #3: remain 0 items');
		   for (var i=0; i < 10; i++) {
		   console.log(i, Cache.get('test'+i));
		   }
		   console.log('\n\n');
		   }, 1600);
