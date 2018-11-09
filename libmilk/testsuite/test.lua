

function mytest(t,v)
	myecho("step 1-1");
	local obj1 = test_enum:new();
	--for k in obj1 do
	--	myecho("ss" .. k);
	--end

	myecho("step 1-2");
	--for k in obj1 do
	--	myecho("ss" .. k);
	--end

	myecho("step 1-3");
	local obj2 = test_system:new(3);

	--for k in obj2 do
	--	myecho("ss" .. k);
	--end


	myecho("step 1-4");
	--obj1.test = "test";
	--myecho(obj1.test);

	myecho("step 1-5");
	--obj2.test = "test";
	--myecho(obj2.test);

	myecho("step 5");
	myecho("IsDebug=" .. tostring(IsDebug));

	return t + v;
end
