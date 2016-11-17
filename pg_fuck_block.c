#include "postgres.h"
#include "fmgr.h"
#include "storage/bufpage.h"
#include "storage/bufmgr.h"
#include "funcapi.h"
#include "nodes/primnodes.h"
#include "utils/rel.h"
#include "access/tuptoaster.h"
#include "catalog/pg_type.h"
#include "utils/snapshot.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(pg_get_page_tuple);
Datum pg_get_page_tuple(PG_FUNCTION_ARGS);


typedef struct _Itemoff{
	OffsetNumber lineoff;
	ItemId	lpp;
	int	lines;
	Page 	page;
	Relation rel; 
} _Itemoff;

Datum pg_get_page_tuple(PG_FUNCTION_ARGS){
	Oid             relid = PG_GETARG_OID(0);
	int64           blkno = PG_GETARG_INT64(1);

	Relation        rel;
	Buffer		buffer;
	Page		page;
	TupleDesc 	tupleDesc;
	_Itemoff 	*itemoff;

	FuncCallContext *funccxt;

	if (SRF_IS_FIRSTCALL()){

		MemoryContext oldcxt;
		int natts = 1;
		rel = relation_open(relid, AccessShareLock);

		if(RelationGetNumberOfBlocks(rel) <= (BlockNumber) (blkno) ) 
          		   elog(ERROR, "block number out of range");


		if (blkno < 0 || blkno > MaxBlockNumber){
			relation_close(rel, AccessShareLock);
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid block number")));
		}

		funccxt = SRF_FIRSTCALL_INIT();	
		oldcxt = MemoryContextSwitchTo(funccxt->multi_call_memory_ctx);


		buffer =  ReadBuffer(rel,blkno);
		LockBuffer(buffer, BUFFER_LOCK_SHARE);

		page = BufferGetPage(buffer);	
		UnlockReleaseBuffer(buffer);

		relation_close(rel, AccessShareLock);

		itemoff = palloc(sizeof(_Itemoff));

		itemoff->lines = PageGetMaxOffsetNumber(page);;
		itemoff->lineoff = FirstOffsetNumber;
		itemoff->lpp = NULL;
		itemoff->page = page;
		itemoff->rel = rel;


		tupleDesc = CreateTemplateTupleDesc(rel->rd_att->natts, false);

		for(;natts <=rel->rd_att->natts;natts++)
			TupleDescInitEntry(tupleDesc, (AttrNumber) natts, NULL, rel->rd_att->attrs[natts-1]->atttypid, -1, 0);

		funccxt->tuple_desc = BlessTupleDesc(tupleDesc);
		funccxt->user_fctx = itemoff;

		MemoryContextSwitchTo(oldcxt);

	}

	funccxt = SRF_PERCALL_SETUP();
	itemoff = funccxt->user_fctx;

	while(itemoff->lineoff <= itemoff->lines)
	{
		Datum *values;
		bool *isnull;
		
		itemoff->lpp = 	PageGetItemId(itemoff->page, itemoff->lineoff);
		itemoff->lineoff++;


		values = (Datum*)palloc0(sizeof(Datum) * funccxt->tuple_desc->natts );
		isnull = (bool*)palloc0(sizeof(bool) * funccxt->tuple_desc->natts );

		if (ItemIdIsNormal(itemoff->lpp))
		{
			HeapTupleData loctup;	
			HeapTuple tup;	
			Datum result;

			int natts = 1;
			bool nulls;
	
			loctup.t_tableOid = relid;
			loctup.t_data = (HeapTupleHeader) PageGetItem((Page) itemoff->page, itemoff->lpp);
			loctup.t_len = ItemIdGetLength(itemoff->lpp);
			ItemPointerSet(&(loctup.t_self), blkno, itemoff->lineoff);

			for(; natts <=  funccxt->tuple_desc->natts;natts++){
				values[natts-1] = heap_getattr(&loctup, natts, funccxt->tuple_desc, &nulls);
			}

			tup = heap_form_tuple(funccxt->tuple_desc, values, isnull);
			result = HeapTupleGetDatum(tup);

			MemSet(values, 0, sizeof(values));
			MemSet(isnull, 0, sizeof(isnull));

			SRF_RETURN_NEXT(funccxt, result);
		}

	}

	SRF_RETURN_DONE(funccxt);
}




