#include "Problema.h"

Problema::Problema(Data &d, double UB)
{
    //Construçao do mestre
    this->UB = UB;
    bestInteger = std::numeric_limits<double>::infinity();
    this->data = d;

    env1 = IloEnv();
    env2 = IloEnv();
    masterModel = IloModel(env1);
    lambda = IloNumVarArray(env1, data.getNItems(), 0.0, IloInfinity);
    masterRanges = IloRangeArray(env1);

    IloExpr sum(env1);
    for (int i = 0; i < data.getNItems(); i++)
    {
        sum += lambda[i];
        masterRanges.add(lambda[i] == 1);
    }

    masterModel.add(masterRanges);
    masterObj = IloObjective(env1);
    masterObj = IloMinimize(env1, sum);
    masterModel.add(masterObj);

    master = IloCplex(masterModel);

    pi = IloNumArray(env1, data.getNItems());

    pricingModel = IloModel(env2);
    x = IloBoolVarArray(env2, data.getNItems());
    IloExpr somaItems(env2);

    for (int i = 0; i < data.getNItems(); i++)
    {
        somaItems += data.getItemWeight(i) * x[i];
    }

    pricingModel.add(somaItems <= data.getBinCapacity());
    pricingObj = IloObjective(env2);
    pricingModel.add(pricingObj);

    pricingConstraints = IloConstraintArray(env2);
    pricingModel.add(pricingConstraints);

    pricing = IloCplex(pricingModel);

    lambdaItens = std::vector<std::vector<int>>(data.getNItems(), {1});

    pricing.setOut(env2.getNullStream());
    master.setOut(env1.getNullStream());

    for (int i = 0; i < data.getNItems(); i++)
    {
        lambdaItens[i][0] = i;
    }

    try
    {
        master.solve();
    }

    catch (IloException &e)
    {
        std::cout << e;
    }

    // for (int i = 0; i < lambda.getSize(); i++)
    // {
    //     std::cout << master.getValue(lambda[i]) << "\n";
    // }
}

std::pair<int, int> Problema::solve(Node &node)
{
    //tratamento de itens juntos

    std::pair<int, int> parAtual;
    if (node.is_root == false)
    {
        std::cout << ((node.tipo_branch == true) ? "Juntos\n" : "Separados\n");
    }
    if (!node.is_root && node.tipo_branch == true)
    {
        int i = node.juntos[node.juntos.size() - 1].first;
        int j = node.juntos[node.juntos.size() - 1].second;

        parAtual = node.juntos[node.juntos.size() - 1];

        x[i].setLB(1.0);
        x[j].setLB(1.0);

        masterObj.setLinearCoef(lambda[i], M);
        masterObj.setLinearCoef(lambda[j], M);
    }

    //tratamento de itens separados
    std::vector<int> colunasProibidas;
    if (!node.is_root && node.tipo_branch == false)
    {
        int i = node.separados[node.separados.size() - 1].first;
        int j = node.separados[node.separados.size() - 1].second;

        // std::cout << "\n\n\n\n"
        //           << i << j << "\n\n\n\n\n";

        parAtual = node.separados[node.separados.size() - 1];

        IloConstraint cons = (x[i] + x[j] <= 1);

        pricingModel.add(cons);
        pricingConstraints.add(cons);

        for (int k = data.getNItems(); k < lambdaItens.size(); k++)
        {
            for (int l = 0; l < lambdaItens[k].size() - 1; l++)
            {
                if ((lambdaItens[k][l] == i && lambdaItens[k][l + 1] == j) || (lambdaItens[k][l] == j && lambdaItens[k][l + 1] == i))
                {
                    // std::cout << i << ", " << j << "\n\n";
                    lambda[k].setUB(0);
                    colunasProibidas.push_back(k);
                }
            }
        }
    }

    xPares = std::vector<std::vector<double>>(data.getNItems(), std::vector<double>(data.getNItems(), 0));

    try
    {
        master.solve();
    }
    catch (IloException &e)
    {
        std::cout << e;
    }

    bool gerouColuna = false;

    while (1)
    {
        master.getDuals(pi, masterRanges);

        IloExpr somaPricing(env2);

        for (int i = 0; i < data.getNItems(); i++)
        {
            somaPricing += pi[i] * x[i];
        }

        pricingObj.setExpr(1 - somaPricing);

        somaPricing.end();

        pricing.setOut(env2.getNullStream());

        try
        {
            pricing.solve();
        }

        catch (IloException &e)
        {
            std::cout << e;
        }

        if (pricing.getStatus() == IloAlgorithm::Infeasible)
        {
            std::cout << "Infeasible pricing\n";
            if (!node.tipo_branch && !node.is_root)
            {
                pricingModel.remove(pricingConstraints[pricingConstraints.getSize() - 1]);
                for (auto &k : colunasProibidas)
                {
                    lambda[k].setUB(1.0);
                }
            }
            if (node.tipo_branch && !node.is_root)
            {

                int i = node.juntos[node.juntos.size() - 1].first;
                int j = node.juntos[node.juntos.size() - 1].second;

                x[i].setLB(0.0);
                x[j].setLB(0.0);

                masterObj.setLinearCoef(lambda[i], 1.0);
                masterObj.setLinearCoef(lambda[j], 1.0);
            }

            std::cout << "par: " << 0 << ", " << 0 << "\n\n\n\n";
            return {0, 0};
            break;
        }

        std::cout << "pricing obj value: " << pricing.getObjValue() << "\n";

        IloNumArray x_vals(env2, data.getNItems());
        pricing.getValues(x_vals, x);

        if (pricing.getObjValue() < -EPSILON)
        {

            gerouColuna = true;

            lambda.add(IloNumVar(masterObj(1) + masterRanges(x_vals), 0.0, IloInfinity));

            std::vector<int> itens;

            // std::cout << "itens pricing : ";
            for (int i = 0; i < x_vals.getSize(); i++)
            {
                if (x_vals[i] > 1 - EPSILON)
                {
                    // std::cout << i << " ";
                    itens.push_back(i);
                }
            }
            // std::cout << "\n";

            lambdaItens.push_back(itens);

            master.setOut(env1.getNullStream());
            try
            {
                master.solve();
            }
            catch (IloException &e)
            {
                std::cout << e;
            }

            x_vals.end();
        }
        else
        {
            x_vals.end();

            break;
        }
    }

    for (int i = 0; i < lambda.getSize(); i++)
    {
        // std::cout << i << ", " << master.getValue(lambda[i]) << "\n";
    }

    std::cout << "Status: " << master.getStatus() << "\n";
    std::cout << "Bins usados: " << master.getObjValue() << "\n";

    //branching rule

    IloNumArray lambdaVals(env1, lambda.getSize());
    master.getValues(lambdaVals, lambda);
    double deltaFrac = std::numeric_limits<double>::infinity();
    double tempDeltaFrac;

    std::vector<double> lambdaPares((data.getNItems() * data.getNItems() - data.getNItems()) / 2);

    for (int i = data.getNItems(); i < lambdaItens.size(); i++)
    {
        for (int j = 0; j < lambdaItens[i].size() - 1; j++)
        {
            xPares[lambdaItens[i][j]][lambdaItens[i][j + 1]] = (xPares[lambdaItens[i][j + 1]][lambdaItens[i][j]] += lambdaVals[i]);
        }
    }

    std::pair<int, int> branchingPair;

    for (int i = 0; i < data.getNItems(); i++)
    {
        for (int j = i + 1; j < data.getNItems(); j++)
        {

            tempDeltaFrac = std::abs(0.5 - xPares[i][j]);
            if (tempDeltaFrac < deltaFrac)
            {
                branchingPair = {i, j};

                if (!node.is_root)
                {
                    if (std::find(node.juntos.begin(), node.juntos.end(), branchingPair) != node.juntos.end() || std::find(node.separados.begin(), node.separados.end(), branchingPair) != node.separados.end())
                    {
                        continue;
                    }
                }

                deltaFrac = tempDeltaFrac;
            }
        }
    }

    lambdaVals.end();

    master.exportModel("modelo.lp");
    pricing.exportModel("pricing.lp");

    //melhor solucao inteira

    //Podar

    if (!gerouColuna)
    {
        std::cout << "Não gerou coluna \n";
    }
    if (gerouColuna)
    {
        std::cout << "Gerou colunas \n";
    }

    if (std::abs(0.5 - deltaFrac) < EPSILON)
    {

        if (std::abs(0.5 - deltaFrac) < EPSILON)
        {
            if (master.getObjValue() < bestInteger)
            {
                bestInteger = master.getObjValue();
            }
        }
        if (!node.tipo_branch && !node.is_root)
        {
            pricingModel.remove(pricingConstraints[pricingConstraints.getSize() - 1]);
            for (auto &k : colunasProibidas)
            {
                lambda[k].setUB(1.0);
            }
        }
        if (node.tipo_branch && !node.is_root)
        {

            int i = node.juntos[node.juntos.size() - 1].first;
            int j = node.juntos[node.juntos.size() - 1].second;

            x[i].setLB(0.0);
            x[j].setLB(0.0);

            masterObj.setLinearCoef(lambda[i], 1.0);
            masterObj.setLinearCoef(lambda[j], 1.0);
        }

        std::cout << "par: " << 0 << ", " << 0 << "\n\n\n\n";
        return {0, 0};
    }

    std::cout << "par: " << branchingPair.first << ", " << branchingPair.second << "\n\n\n\n";

    return branchingPair;
}
