from Numberjack import *


class MIPParser(object):
    def __init__(self, model_path, data_path=None,
                       solver_name=None, verbose=0):
        self.model_path = model_path
        self.data_path = data_path
        self.solver_name = solver_name
        self.model = None
        self.verbose = verbose
        self.vars = {}

    def parse_model(self):
        # Import Glpk solver so that we have support for gmpl
        import OsiGlpk as Osi

        parts = self.model_path.split('.')
        extensions = parts[-2:len(parts)]
        filetype = extensions[0] if extensions[1] == 'gz' else extensions[1]

        parser = Osi.Solver(Model())
        if filetype == 'mod':
            parser.load_gmpl(self.model_path, self.data_path)
        elif filetype == 'mps':
            parser.load_mps(self.model_path, 'mps')
        elif filetype == 'lp':
            parser.load_lp(self.model_path, 0)
        else:
            print "Unknown filetype, Supported formats are [gmpl, mps, mps.gz, lp], exiting"
            return None
        prse = parser.solver

        self.model = Model()

        n = prse.num_expression()
        for i in range(n):
            expr = self.getNJExp(prse.get_expression(i), 0)
            self.model.add(expr)
        return self.model

    def solve(self):
        if self.model == None:
            self.parse_model()
        solver = self.model.load(self.solver_name)
        solver.setVerbosity(self.verbose)
        solver.solve()
        return (solver, self.model)

    def getNJExp(self, mexp, ident):
        exp_type = mexp.get_type()
        if exp_type == 'var':
            return self.getNJVar(mexp, ident)
        else:
            return self.getNJPred(mexp)

    def ifwhole(self, val):
        return val if val % 1 != 0 else int(val)

    def getNJPred(self, mexp):
        exp_type = mexp.get_type()

        children = []
        for i in range(mexp.get_arity()):
            children.append(self.getNJExp(mexp.get_child(i), 0))

        if exp_type == "le":
            return children[0] <= self.ifwhole(mexp.get_parameter(0))
        elif exp_type == "ge":
            return children[0] >= self.ifwhole(mexp.get_parameter(0))
        elif exp_type == "sum":
            weights = []
            for i in range(mexp.get_arity()):
                weights.append(self.ifwhole(mexp.get_parameter(i)))
            return Sum(children, weights)
        elif exp_type == "minimise":
            return Minimise(children[0])
        else:
            print "Error: Failed to parse expression:", type
            exit(1)

    def getNJVar(self, mexp, ident):
        ident = mexp.getVariableId()
        if ident not in self.vars:
            lb = mexp.get_min() if mexp.is_continuous() else int(mexp.get_min())
            ub = mexp.get_max() if mexp.is_continuous() else int(mexp.get_max())
            self.vars[ident] = Variable(lb, ub, mexp.name())
        return self.vars[ident]


if __name__ == "__main__":
    import os
    import sys
    from optparse import OptionParser

    opts = OptionParser()
    opts.set_usage('Usage: FILE... [OPTION]...\n')
    opts.add_option("--data", "-d", help="data file for gmpl")
    opts.add_option("--solver", "-s", type="choice", choices=["Mistral",
        "MiniSat", "Walksat", "SCIP", "OsiClp", "OsiCbc", "OsiGlpk",
        "OsiVol", "OsiDylp", "OsiSpx", "OsiGrb", "OsiSym"],
        help="Mistral|MiniSat|Walksat|SCIP|OsiClp|OsiCbc|OsiGlpk|OsiVol|OsiDylp|OsiSpx|OsiGrb|OsiSym")
    opts.add_option("--verbosity", "-v", help="verbosity level for solve")
    
    options, arguments = opts.parse_args()

    datafile = options.data if options.data else None
    solver_name = options.solver if options.solver else "OsiCbc"
    verbose = int(options.verbosity) if options.verbosity else 0
    
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    
        parser = MIPParser(filename, datafile, solver_name, verbose)
        (solver, model) = parser.solve()
        if solver.is_sat():
            nodes = solver.getNodes()
            time = solver.getTime()
            nodesps = nodes/time if time > 0 else nodes
            
            print("c Result                      :         %11s" % 'SAT')
            print("c #Nodes                      :         %11s" % nodes)
            print("c Solving time (in sec.)      :         %11s" % time)
            print("c Nodes/second                :         %11s" % nodesps)
            print 'v', ' '.join(str(v.get_value()) for v in model.variables)
        else:
            print "UNSAT"
    else:
        opts.print_help()
