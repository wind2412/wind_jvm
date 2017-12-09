import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.util.Arrays;
import java.util.List;

class Test8 {
	public static void main(String[] args) throws Throwable {  
		MethodHandles.Lookup lookup = MethodHandles.lookup();  
		MethodHandle mh = lookup.findStatic(Test8.class, "doubleVal", MethodType.methodType(int.class, int.class));  
		List<Integer> dataList = Arrays.asList(1, 2, 3, 4, 5);  
		Test8.transform(dataList, mh);// 方法做为参数  
		for (Integer data : dataList) {  
			System.out.println(data);//2,4,6,8,10  
		}  
	}  
	public static List<Integer> transform(List<Integer> dataList, MethodHandle handle) throws Throwable {  
		for (int i = 0; i < dataList.size(); i++) {  
			dataList.set(i, (Integer) handle.invoke(dataList.get(i)));//invoke  
		}  
		return dataList;  
	}  
	public static int doubleVal(int val) {  
		return val * 2;  
	}  
}