#include "Graph.h"
#include "data.h"
#include "MyLazyCallback.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <time.h>
#include <cmath>
#include <limits>

#include <ilcplex/ilocplex.h>

int main(int argc, char **argv)
{

	Data *d = new Data(argc, argv[1]);
	d->readData();
	d->printMatrixDist();
	std::cout << "\n";

	Graph *G = new Graph(d->getDimension(), d->getMatrixCost());
	// G->printEdges();
	int N = G->V();

	//Modelo
	IloEnv env;
	IloModel modelo(env);

	//Variaveis
	IloBoolVarArray x(env, G->E());

	for (int e = 0; e < G->E(); e++)
	{
		modelo.add(x[e]);
	}

	//Restriçoes de grau
	for (int i = 0; i < N; i++)
	{
		IloExpr sum(env);
		for (int j = 0; j < N; j++)
		{
			if (i == j)
				continue;

			int e = G->getEdge(i, j);
			sum += x[e];
		}
		modelo.add(sum == 2);
	}

	// FO
	{
		IloExpr sumEdges(env);
		for (int e = 0; e < G->E(); e++)
		{
			Edge &ed = G->getEdges()[e];
			sumEdges += x[e] * ed.w;
		}
		modelo.add(IloMinimize(env, sumEdges));
	}

	IloCplex TSP(modelo);

	IloBoolVarArray &x_ref = x;

	MyLazyCallback *lazyCbk = new (env) MyLazyCallback(env, x_ref, G);
	TSP.use(lazyCbk);

	try
	{
		TSP.solve();
	}

	catch (IloException &e)
	{
		std::cout << e;
	}

	std::cout << "status: " << TSP.getStatus() << std::endl;
	std::cout << "custo:" << TSP.getObjValue() << std::endl;

	// for (int e = 0; e < G->E(); e++)
	// {
	// 	if (TSP.getValue(x[e]) > 0.9)
	// 	{
	// 		Edge &ed = G->getEdges()[e];
	// 		std::cout << ed.i << ", " << ed.j << "\n";
	// 	}
	// }

	return 0;
}
