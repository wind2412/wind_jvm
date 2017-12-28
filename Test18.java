import java.util.*;
import java.util.stream.*;

// 此 test 会失败。由于 Lookup 的调用在我 vm 中的实现相关问题。

class Test18 {
	public static void main(String[] args) {
		// 将字符串换成大写并用逗号链接起来
		List<String> G7 = Arrays.asList("USA", "Japan", "France", "Germany", "Italy", "U.K.","Canada");
		String G7Countries = G7.stream().map(x -> x.toUpperCase()).collect(Collectors.joining(", "));
		System.out.println(G7Countries);
	}
}