import java.util.*;

class Test9 {
	
	public static void main(String[] args) {
		List<Object> l = new ArrayList<>();
		l.add("hhaa");
		String[] aa = new String[5];
		l.toArray(aa);
		for (int i = 0; i < aa.length; i ++) {
			System.out.println(aa[i]);
		}
	}
}