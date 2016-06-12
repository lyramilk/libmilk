local t = os:new();
t:print("[lua]加载脚本开始");


function test(v)
	t:print("[lua]执行test"..v);
	local m = {};
	m.app = "map_app";
	m.des = "map_des";
	m.tototo = "map_tototo";


	local w = niuniu:new();
	t:print("[lua]"..w:add(1997));
	return w:testmap(m);
end

t:print("[lua]加载脚本完成");
