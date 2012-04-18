class CConnectionIterator
{
public:
	CConnectionIterator( cnet * net );
	~CConnectionIterator();
	int First();
	int Last();
	int Next();
	int Prev();
	void Removed( int ic );
	void Added();

	cnet * m_net;	// net that "owns" this iterator
	int current_ic;	// index to current connection, or -1 if current connection has been removed
	int prev_ic;	// index to previous connection, or -1 if at beginning of array
	int next_ic;	// index to next connection, or -1 if at end of array
};

