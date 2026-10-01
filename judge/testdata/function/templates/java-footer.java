
public static void main(String[] args) throws IOException {
    StreamTokenizer in = new StreamTokenizer(new BufferedReader(new InputStreamReader(System.in)));
    in.nextToken(); int n = (int) in.nval;
    int[] a = new int[n];
    for (int i = 0; i < n; i++) { in.nextToken(); a[i] = (int) in.nval; }
    System.out.println(maxPairSum(a));
}
}
