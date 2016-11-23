local t1 = test_system:new(3);
t1:print("[lua]加载脚本开始");
function mytest(t,v)
	myecho("[lua]执行test");

	local m = {};
	m.app = "map_app";
	m.des = "map_des";
	m.tototo = "map_tototo";

	t:print("[lua]"..v:add(1987));
	return v:testmap(m);
end
t1:print("[lua]加载脚本完成");
