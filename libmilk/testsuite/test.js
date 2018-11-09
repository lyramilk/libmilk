
function mytest(t,v)
{
	myecho("step 1-1");

	let obj1 = new test_enum();
	for(let k in obj1){
		myecho("ss" + k);
	}

	myecho("step 1-2");
	for(let k in obj1){
		myecho("ss" + k);
	}
	myecho("step 1-3");

	let obj2 = new test_system(3);

	for(let k in obj2){
		myecho(k);
	}
	myecho("step 1-4");

	obj1.test = "test";
	myecho(obj1.test);

	myecho("step 1-5");

	obj2.test = "test";
	myecho(obj2.test);

	myecho("step 2");
	myecho(t);

	myecho("step 3");
	myecho(v);

	myecho("step 4");
	myecho(t== v?"true":"false");


	myecho("step 5");
	myecho("IsDebug="+IsDebug);

	return t + v;
}