var t = new os();
t.print("[js]加载脚本开始。");
//var w = new niuniu(2002);
function test(v,w)
{
	t.print("[js]执行test" + v);

	var m = {
		"app":"map_app",
		"des":"map_des",
		"tototo":"map_tototo",
	};

	t.print("[js]" + w.add(1987));
	w.testmap(m);
	return t;
}

t.print("[js]加载脚本完成。");
