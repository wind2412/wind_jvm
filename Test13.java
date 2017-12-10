// These codes are from: jdk: abstract class AbstractValidatingLambdaMetafactory

interface III<T> {
	Object foo(T x); 
}
interface JJ<R extends Number> extends III<R> {
	
}
class CC { 
	String impl(int i) { 
		return "impl:"+i; 
	}
	String impl(float d) {
		return "";
	}
}

public class Test13 {

	
	public static void main(String[] args) {
		JJ<Integer> iii = (new CC())::impl;
//		JJ<Float> r = (new CC())::impl;
		System.out.printf(">>> %s\n", iii.foo(44));
//		System.out.printf(">>> %s\n", iii.foo(3.5));		// 并不能运作？？？
	}
	
}
