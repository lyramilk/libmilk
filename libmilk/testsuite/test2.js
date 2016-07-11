
var t = new os(8000);
t.print("第0次");

var t1 = new os(100000);
t1.print("第一次");
var t2 = new os(200000);
t1.print("第二次");
var q1 = new niuniu(300000);
t1.print("第三次");
var q2 = new niuniu(400000);
t1.print("第四次");

"s";

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
	return w.testmap(m);
}

t.print("[js]加载脚本完成。");
