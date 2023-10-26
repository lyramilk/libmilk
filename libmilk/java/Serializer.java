/**
	@namespace lyramilk.data
	@brief 数据
	@details 该命名空间描述数据的表达形式。
*/
package lyramilk.data;

import java.io.*;
import java.util.*;
import java.lang.reflect.*;

import java.lang.annotation.Annotation;
import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.annotation.ElementType;


/** 
 * @brief 序列化工具。
 * @details 兼容C++ libmilk库中的libmilk::data::var
 * @author lyramilk
 */
@SuppressWarnings({ "unchecked"})
public class Serializer {
	/*
	 * @brief 这是一个注解，这个注解可以用来标记可序列化的类或类中的字段。
	 * @details 不使用该标记的情况下仅支持String,Boolean,Character,Byte,Short,Integer,Long,Float,Double,byte[],Iterable,Map及它们子类的组合。
	 */
	@Target({ElementType.FIELD,ElementType.TYPE})
	@Retention(RetentionPolicy.RUNTIME)
	public @interface Serializable {
		String value() default "";
	}

	/** 
	 * @brief 将对象序列化后存储到流中。
	 * @param os 输出流，对象序列化后将输出到该流中。输出过程不会缓冲，谨慎直接使用通信代价较大的流，如SocketOutputStream。。一般情况下建议使用ByteArrayOutputStream做缓冲。
	 * @param obj 被序列化的对象。
	 * @return 返回序列化是否成功。如果不成功，则写入到流中的数据都是无效的。
	 */
	public static boolean serialize(OutputStream os, Object obj) throws IOException {
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		DataOutputStream dbos = new DataOutputStream(bos);
		if (_serialize(dbos, obj)) {
			dbos.flush();
			DataOutputStream dos = new DataOutputStream(os);
			byte[] bytes = bos.toByteArray();
			dos.writeInt(bytes.length);
			dos.write(bytes);
			dos.flush();
			return true;
		}
		return false;
	}


	/** 
	 * @brief 从流中串行化成对象。
	 * @param is 输入流。串行化过程中会多次读取该流，谨慎直接使用通信代价较大的流，如SocketInputStream。。一般情况下建议使用ByteArrayInputStream做缓冲。
	 * @return 返回生成好的对象，返回null表示串行化失败。
	 */
	public static Object deserialize(InputStream is) throws IOException,TypeUndefine {
		if (is.markSupported()) {
			is.mark(is.available());
		}
		DataInputStream dis = new DataInputStream(is);
		int objsize = dis.readInt();
		if (objsize > dis.available()) {
			is.reset();
			return null;
		}
		Object ret = _deserialize(dis);
		if (ret != null)
			return ret;
		is.reset();
		return null;
	}

	/** 
	 * @brief 将流串行化为对象。
	 * @param is 输入流。串行化过程中会多次读取该流，谨慎直接使用通信代价较大的流，如SocketInputStream。。一般情况下建议使用ByteArrayInputStream做缓冲。
	 * @param t 指定类型的对象，串行化时会将数据内容填充到t中，将t中所有带有串行化注解的赋值。
	 * @return 返回生成好的对象，返回null表示串行化失败。
	 */
	public static <T> T deserialize(InputStream is,T t) throws IOException,TypeUndefine {
		Object o = deserialize(is);
		return deserialize(o,t);
	}

	/** 
	 * @brief 将从流中串行化的对象转化为对象。
	 * @param o 输入对象
	 * @param t 输出类型
	 * @return 返回生成好的对象，返回null表示串行化失败。
	 */
	public static <T> T deserialize(Object o,T t) throws IOException,TypeUndefine {
		Class cls = t.getClass();
		if(o instanceof Map){
			Map m = (Map)o;

			Field[] fs = cls.getFields();
			for(Field f : fs){
				Serializer.Serializable s = f.getAnnotation(Serializer.Serializable.class);
				if(s != null){
					try{
						String sname = s.value();
						if(sname == null || sname.equals("")){
							sname = f.getName();
						}
						f.setAccessible(true);
						f.set(t,m.get(sname));
					}catch(IllegalAccessException e){
						e.printStackTrace();
					}
				}
			}
		}else{
			t = (T)o;
		}
		return t;
	}

	/** 
	 * @brief 取得串行化对象所需的数据量。
	 * @param is 输入流。
	 * @return 对象的大小
	 */
	public static int expect(InputStream is) throws IOException {
		if (is.markSupported()) {
			is.mark(is.available());

			DataInputStream dis = new DataInputStream(is);
			int objsize = dis.readInt();
			is.reset();

			return objsize;
		}
		return -1;
	}


	private static boolean _serialize(DataOutputStream dos, Object obj)
			throws IOException {
		byte magic = (byte) 0x80;
		if (obj instanceof String) {
			String v = (String) obj;
			magic = (byte) 0x80 | t_str;
			dos.writeByte(magic);
			byte[] buff = v.getBytes("utf-8");
			dos.writeInt(buff.length);
			dos.write(buff);

		} else if (obj instanceof Boolean) {
			magic = (byte) 0x80 | t_bool;
			Boolean b = (Boolean) obj;
			char c = (char) (b ? 1 : 0);
			dos.writeByte(magic);
			dos.writeByte(c);
		} else if (obj instanceof Character) {
			magic = (byte) 0x80 | t_int8;
			Character c = (Character) obj;
			dos.writeByte(magic);
			dos.writeByte(c);
		} else if (obj instanceof Byte) {
			magic = (byte) 0x80 | t_uint8;
			Byte c = (Byte) obj;
			dos.writeByte(magic);
			dos.writeByte(c);
		} else if (obj instanceof Short) {
			magic = (byte) 0x80 | t_int16;
			Short c = (Short) obj;
			dos.writeByte(magic);
			dos.writeByte(c);
		} else if (obj instanceof Integer) {
			magic = (byte) 0x80 | t_int32;
			Integer c = (Integer) obj;
			dos.writeByte(magic);
			dos.writeInt(c);
		} else if (obj instanceof Long) {
			magic = (byte) 0x80 | t_int64;
			Long c = (Long) obj;
			dos.writeByte(magic);
			dos.writeLong(c);
		} else if (obj instanceof Float) {
			magic = (byte) 0x80 | t_double;
			Float c = (Float) obj;
			dos.writeByte(magic);
			dos.writeDouble(c);
		} else if (obj instanceof Double) {
			magic = (byte) 0x80 | t_double;
			Double c = (Double) obj;
			dos.writeByte(magic);
			dos.writeDouble(c);
		} else if (obj instanceof byte[]) {
			/*byte[]也是Iterable的子类，所以一定要在Iterable之前*/
			magic = (byte) 0x80 | t_bin;
			byte[] buff = (byte[])obj;
			dos.writeByte(magic);
			dos.writeInt(buff.length);
			dos.write(buff);
			return true;
		} else if (obj instanceof Iterable) {
			magic = (byte) 0x80 | t_array;

			dos.writeByte(magic);
			Iterable c = (Iterable) obj;
			short i = 0;
			for (@SuppressWarnings("unused") Object o : c) {
				i++;
			}
			dos.writeShort(i);

			for (Object o : c) {
				if (!_serialize(dos, o)) {
					return false;
				}
			}
		} else if (obj instanceof Map) {
			magic = (byte) 0x80 | t_map;
			dos.writeByte(magic);
			
			Map m = (Map) obj;
			short i = (short)m.size();
			dos.writeShort(i);

			for (Object eo : m.entrySet()) {
				Map.Entry<Object, Object> e = (Map.Entry<Object, Object>) (eo);
				Object key = e.getKey();
				Object value = e.getValue();
				if (!(_serialize(dos, key) && _serialize(dos, value))) {
					return false;
				}
			}
		}else{
			if(obj == null){
				magic = (byte) 0x80 | t_invalid;
				dos.writeByte(magic);
				return true;
			}
			Class cls = obj.getClass();
			Annotation a = cls.getAnnotation(Serializer.Serializable.class);
			if(a == null){
				throw new IllegalArgumentException(Serializer.class.getName() + "不支持序列化" + obj.getClass().getName());
			}

			magic = (byte) 0x80 | t_map;
			dos.writeByte(magic);
			Map m = new HashMap<Integer,Integer>();
			Field[] fs = cls.getFields();
			for(Field f : fs){
				Serializer.Serializable s = f.getAnnotation(Serializer.Serializable.class);
				if(s != null){
					try{
						String sname = s.value();
						if(sname == null || sname.equals("")){
							sname = f.getName();
						}
						Object key = sname;
						f.setAccessible(true);
						Object value = f.get(obj);
						m.put(key,value);
					}catch(IllegalAccessException e){
						e.printStackTrace();
					}
				}
			}

			short i = (short)m.size();
			dos.writeShort(i);

			for (Object eo : m.entrySet()) {
				Map.Entry<Object, Object> e = (Map.Entry<Object, Object>) (eo);
				Object key = e.getKey();
				Object value = e.getValue();
				if (!(_serialize(dos, key) && _serialize(dos, value))) {
					return false;
				}
			}
		}
		return true;
	}
	
	private static Object _deserialize(DataInputStream dis) throws IOException,TypeUndefine {
		byte magic = dis.readByte();
		boolean r = (magic & 0x80) == 0;
		byte t = (byte) (magic & 0x7f);
		switch (t) {
		case t_bin:{
			int size = dis.readInt();
			if (r)
				size = reverse_order(size);
			byte[] buff = new byte[size];
			dis.read(buff);
			return buff;
		}
		case t_str:
		case t_wstr: {
			int size = dis.readInt();
			if (r)
				size = reverse_order(size);
			byte[] buff = new byte[size];
			dis.read(buff);
			return new String(buff, "utf-8");
		}
		case t_bool: {
			byte c = dis.readByte();
			if (r)
				c = reverse_order(c);
			boolean b = c == 1;
			return b;
		}
		case t_int8: {
			if (r)
				return reverse_order(dis.readByte());
			return dis.readByte();
		}
		case t_uint8: {
			if (r)
				return reverse_order(dis.readByte());
			return dis.readByte();
		}
		case t_int16:
		case t_uint16: {
			if (r)
				return reverse_order(dis.readShort());
			return dis.readShort();
		}
		case t_int32:
		case t_uint32: {
			if (r)
				return reverse_order(dis.readInt());
			return dis.readInt();
		}
		case t_int64:
		case t_uint64: {
			if (r)
				return reverse_order(dis.readLong());
			return dis.readLong();
		}
		case t_double: {
			return dis.readDouble();
		}
		case t_array: {
			short size = dis.readShort();
			if (r)
				size = reverse_order(size);
			ArrayList a = new ArrayList();
			for (int i = 0; i < size; ++i) {
				Object item = _deserialize(dis);
				if (item == null)
					return null;
				a.add(item);
			}
			return a;
		}
		case t_map: {
			short size = dis.readShort();
			if (r)
				size = reverse_order(size);
			Map a = new HashMap();
			for (int i = 0; i < size; ++i) {
				Object key = _deserialize(dis);
				if (key == null)
					return null;
				Object value = _deserialize(dis);
				if (value == null)
					return null;
				a.put(key,value);
			}
			return a;
		}		
		case t_invalid:
			return null;
		default:
			throw new TypeUndefine(Serializer.class.getName() + "串行化时遇到不支持的子类型" + ((int)t));
		}
	}

	final static byte t_invalid = 0x20;
	final static byte t_bin = 0x50;
	final static byte t_str = 0x30;
	final static byte t_wstr = 0x31;
	final static byte t_bool = 0x21;
	final static byte t_int8 = 0x40;
	final static byte t_uint8 = 0x41;
	final static byte t_int16 = 0x42;
	final static byte t_uint16 = 0x43;
	final static byte t_int32 = 0x44;
	final static byte t_uint32 = 0x45;
	final static byte t_int64 = 0x46;
	final static byte t_uint64 = 0x47;
	final static byte t_double = 0x48;
	final static byte t_array = 0x51;
	final static byte t_map = 0x52;

	static short reverse_order(short s) {
		return Short.reverseBytes(s);
	}

	static int reverse_order(int s) {
		return Integer.reverseBytes(s);
	}

	static long reverse_order(long s) {
		return Long.reverseBytes(s);
	}

	static char reverse_order(char s) {
		return s;
	}

	static byte reverse_order(byte s) {
		return s;
	}



	/*
	 * @brief 测试函数
	 */
	public static void main(String args[]) throws Exception{
		Map m = new HashMap<Object,Object>();
		m.put("rpc.cmd", "size");
		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		Serializer.serialize(bos, m);

		ByteArrayInputStream bis = new ByteArrayInputStream(bos.toByteArray());
		Object obj = Serializer.deserialize(bis);
	}
}