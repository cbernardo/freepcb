/* CConnectionIterator implementation

This class is sort of a "super-iterator" for moving through the
array of connections in a net
Unlike standard iterators, it can be used by processes which 
add or remove connections without losing its place
For this to work, any function that adds or removes connections has to 
notify the iterator

Rules, assuming m_net->nconnects > 0:
	if current_ic == -1, the current connection was removed or never selected, 
		but the position of the iterator is still maintained by prev_ic and next_ic
	if prev_ic == -1, we are at the beginning of the connection array
	if next_ic == -1, we are at the end of the connection array
	if an external process removes a connection, it must call Removed(ic)
	if an external process adds a connection, it must call Added()

Example:
	// remove any connections with more than 5 segments
	CConnectionIterator cci( net );
	for( int ic=cci.First(); ic=cci.Next(); ic!=-1 )
	{
		cconnect * c = &net->connect[ic];
		if( c->nsegs > 5 )
		{
			net->connect.RemoveAt(ic);
			cci.Removed(ic);
		}
	}
*/

CConnectionIterator::CConnectionIterator( cnet * net )
{
	if( net->m_cci )
		ASSERT(0);	// iterator already exist, error
	// link this iterator with net
	m_net = net;
	net->m_cci = this;	
	// initialize to point to start of array, before first connection
	// calling Next() will select the first connection
	prev_ic = -1;
	current_ic = -1;
	if( m_net->nconnects )
		next_ic = 0;
	else
		next_ic = -1;
}

CConnectionIterator::~CConnectionIterator()
{
	m_net->m_cci = NULL;	//notify net we are gone
}

int CConnectionIterator::First()
{
	prev_ic = -1;
	if( m_net->nconnects == 0 )
	{
		current_ic = -1;
		next_ic = -1;
	}
	else if( m_net->nconnects == 1 )
	{
		current_ic = 0;
		next_ic = -1;
	}
	else
	{
		current_ic = 0;
		next_ic = 1;
	}
	return current_ic;
}

int CConnectionIterator::Last()
{
	next_ic = -1;
	if( m_net->nconnects == 0 )
	{
		prev_ic = -1;
		current_ic = -1;
	}
	else if( m_net->nconnects == 1 )
	{
		current_ic = m_net->nconnects - 1;
		prev_ic = -1;
	}
	else
	{
		current_ic = m_net->nconnects - 1;
		prev_ic = current_ic - 1;
	}
	return current_ic;
}

int CConnectionIterator::Next()
{
	if( m_net->nconnects == 0 )
	{
		prev_ic = -1;
		current_ic = -1;
		next_ic = -1;
	}
	else
	{
		if( next_ic == -1 )
		{
			// we are already at end
			current_ic = -1;
			prev_ic = m_net->nconnects - 1;
		}
		else
		{
			// next_ic is valid, set current_ic = next_ic
			current_ic = next_ic;
			if( current_ic == m_net->nconnects-1 )
				next_ic = -1;
			else
				next_ic = current_ic + 1;
			if( current_ic == 0 )
				prev_ic = -1;
			else
				prev_ic = current_ic - 1;
		}
	}
	return current_ic;
}

int CConnectionIterator::Prev()
{
	if( m_net->nconnects == 0 )
	{
		prev_ic = -1;
		current_ic = -1;
		next_ic = -1;
	}
	else
	{
		if( prev_ic == -1 )
		{
			// we are already at start
			current_ic = -1;
			next_ic = 0;
		}
		else
		{
			// prev_ic is valid, set current_ic = prev_ic
			current_ic = prev_ic;
			if( current_ic == 0 )
				prev_ic = -1;
			else
				prev_ic = current_ic - 1;
			if( current_ic == m_net->nconnects-1 )
				next_ic = -1;
			else
				next_ic = current_ic + 1;
		}
	}
	return current_ic;
}

// should be called after removing a connection
void CConnectionIterator::Removed( int ic )
{
	if( m_net->nconnects == 0 )
	{
		prev_ic = current_ic = next_ic = -1;
	}
	else
	{
		if( ic == current_ic )
		{
			// mark current_ic removed
			// shift next_ic down, if possible
			current_ic = -1;
			if( next_ic > 0 )
				next_ic--;
			else
				next_ic = -1;
		}
		else if( ic == next_ic )
		{
			// shift next_ic up if possible
			if( next_ic < m_net->nconnects-1 )
				next_ic++;
			else
				next_ic = -1;
		}
		else if( ic <= prev_ic )
		{
			// shift all down if possible
			if( prev_ic > 0 )
				prev_ic--;
			else
				prev_ic = -1;
			if( current_ic > 0 )
				current_ic--;
			else
				current_ic = -1;
			if( next_ic > 0 )
				current_ic--;
			else
				next_ic = -1;
		}
	}	
}

