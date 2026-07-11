namespace App
{
    // Minimal nanoFramework application entry point. Compiled headlessly (scripts/build-app-pe.sh) and executed by
    // the WASI reactor (tests/run-app.mjs) to prove nanoCLR runs managed bytecode end-to-end. The loop is interpreted
    // work whose wall-clock only exists if Main actually runs (the end-to-end script also asserts the "Ready."/"Done."
    // lifecycle with no "Cannot find any entrypoint!").
    public static class Program
    {
        public static void Main()
        {
            long accumulator = 0;
            for (int i = 0; i < 1000000; i++)
            {
                accumulator += (i & 15);
            }

            // Keep the result observable to the optimizer without producing host output (there is no Console here).
            if (accumulator == -1)
            {
                throw new System.Exception();
            }
        }
    }
}
