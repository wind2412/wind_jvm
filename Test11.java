class Test11 {
	public static void main(String[] args) {
		new Thread( () -> System.out.println("In Java8, Lambda expression rocks !!") ).start();
		Runnable x = () -> {  
					System.out.println("Hello, World!");   
				};  
		new Thread(x).start();
	}
}