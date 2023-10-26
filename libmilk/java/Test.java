package lyramilk.data;

import java.util.*;
import java.io.*;
import java.lang.reflect.*;
import java.lang.annotation.*;
import lyramilk.data.*;

@Serializer.Serializable
enum U
{
	APPLE,ORANGE
}


@Serializer.Serializable
class E
{
	@Serializer.Serializable("技能名称")
	public String name;

	@Serializer.Serializable("技能效果")
	public String value;

	@Serializer.Serializable("技能颜色")
	U u;
}

@SuppressWarnings({ "unchecked", "rawtypes" })
@Serializer.Serializable
public class Test {
	@Serializer.Serializable("名称")
	private String name;

	@Serializer.Serializable("颜色")
	private String colour;

	@Serializer.Serializable
	private String desc;

	@Serializer.Serializable("技能")
	private ArrayList<E> es = new ArrayList<E>();



	public static void main(String args[]) throws IOException{
		Test q = new Test();
		q.name = "猴子";
		q.colour = "褐色";
		q.desc = "齐天大圣";

		{
			E e = new E();
			e.name = "突进";
			e.value = "跑到你面前";
			e.u = U.APPLE;
			q.es.add(e);
		}

		{
			E e = new E();
			e.name = "大闹天宫";
			e.value = "自转";
			e.u = U.ORANGE;
			q.es.add(e);
		}

		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		Serializer.serialize(bos,q);

		ByteArrayInputStream bis = new ByteArrayInputStream(bos.toByteArray());

		Test o2 = new Test();
		try{
		Serializer.deserialize(bis,o2);
		}catch(TypeUndefine e){

		}

		System.out.println("desc=" + o2.desc);
		System.out.println("name=" + o2.name);
		System.out.println("es=" + o2.es);
	}
}
