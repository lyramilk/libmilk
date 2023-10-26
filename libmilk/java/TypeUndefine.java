package lyramilk.data;
public class TypeUndefine extends Exception
{
	public TypeUndefine()
	{}
	public TypeUndefine(String message)
	{
		super(message);
	}
	public TypeUndefine(String message, Throwable cause)
	{
		super(message,cause);
	}
	public TypeUndefine(Throwable cause)
	{
		super(cause);
	}
}