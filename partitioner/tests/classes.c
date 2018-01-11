struct myclass {
	int awesome;
	float tubular;
};

struct otherclass {
	double radical;
	double gnarly;
};

struct otherclass globVar;

int myfunc(struct myclass var)
{
	int i;
#pragma omp parallel for
	for(i = 0; i < 10; i++)
	{
		globVar.radical = (double)var.tubular;
		globVar.gnarly = globVar.gnarly + (double)var.awesome;
	}
	return var.awesome;
}

int main(int argc, char** argv)
{
	struct myclass var = {42, 3.14};
	myfunc(var);
}
