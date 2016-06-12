var t = new os();
t.print("[js]加载脚本开始。");
function test(v)
{
	t.print("[js]执行test" + v);

	var m = {
		"app":"map_app",
		"des":"map_des",
		"tototo":"map_tototo",
	};

	var w = new niuniu(2002);
	t.print("[js]" + w.add(1987));
	return w.testmap(m);
}

t.print("[js]加载脚本完成。");
