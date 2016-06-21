local t = os:new(200000);
t:print("[lua]加载脚本开始");

--local w = niuniu:new();
function test(v,w)
	t:print("[lua]执行test"..v);
	local m = {};
	m.app = "map_app";
	m.des = "map_des";
	m.tototo = "map_tototo";


	t:print("[lua]"..w:add(1997));
	w:testmap(m);
	return t;
end

t:print("[lua]加载脚本完成");
