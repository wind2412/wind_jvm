class Test10 {
	public static void main(String[] args) {
		System.out.println(String.class.getName());
		System.out.println(byte.class.getName());
		System.out.println((new Object[3]).getClass().getName());
		System.out.println((new int[3][4][5][6][7][8][9]).getClass().getName());
//		returns "java.lang.String"
//		returns "byte"
//		returns "[Ljava.lang.Object;"
//		returns "[[[[[[[I"
	}
}