import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

class BB {
	private final int i;

	public BB(int i_) {
		i = i_;
	}

	private void bar(int j, String msg) {
		System.out.println("Private Method 'bar' successfully accessed : " + i + ", " + j + " : " + msg + "!");
	}

	// Using Reflection
	public static Method makeMethod() {
		Method meth = null;

		try {
			Class[] argTypes = new Class[] { int.class, String.class };

			meth = BB.class.getDeclaredMethod("bar", argTypes);

			meth.setAccessible(true);
		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		} catch (SecurityException e) {
			e.printStackTrace();
		}

		return meth;
	}

	// Using method handles
	public static MethodHandle makeMh() {
		MethodHandle mh = null;

		MethodType desc = MethodType.methodType(void.class, int.class, String.class);

		try {
			mh = MethodHandles.lookup().findVirtual(BB.class, "bar", desc);
		} catch (NoSuchMethodException | IllegalAccessException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		System.out.println("mh=" + mh);

		return mh;
	}
}

public class Test4 {

	private static void withReflectionArgs() {
		Method meth = BB.makeMethod();

		BB mh0 = new BB(0);
		BB mh1 = new BB(1);

		System.out.println("Invocation using Reflection");
		try {
			meth.invoke(mh0, 5, "Jabba the Hutt");
			meth.invoke(mh1, 7, "Boba Fett");
		} catch (IllegalAccessException | IllegalArgumentException | InvocationTargetException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}

	private static void withMhArgs() {
		MethodHandle mh = BB.makeMh();

		BB mh0 = new BB(0);
		BB mh1 = new BB(1);

		try {
			System.out.println("Invocation using Method Handle");
			mh.invokeExact(mh0, 42, "R2D2");
			mh.invokeExact(mh1, 43, "C3PO");
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}

	public static void main(String[] args) {
		withReflectionArgs();
		withMhArgs();
	}
}