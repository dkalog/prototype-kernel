/*
 * Benchmarking page allocator execution time inside the kernel
 *
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/time.h>
#include <linux/time_bench.h>
#include <linux/spinlock.h>
#include <linux/mm.h>

static int verbose=1;

static int time_single_page_alloc_put(
	struct time_bench_record *rec, void *data)
{
	gfp_t gfp_mask = (GFP_ATOMIC | ___GFP_NORETRY);
	struct page *my_page;
	int i;

	time_bench_start(rec);
	/** Loop to measure **/
	for (i = 0; i < rec->loops; i++) {
		my_page = alloc_page(gfp_mask);
		if (unlikely(my_page == NULL))
			return 0;
		__free_page(my_page);
	}
	time_bench_stop(rec, i);
	return i;
}

static int time_alloc_pages(
	struct time_bench_record *rec, void *data)
{
	/* Important to set: __GFP_COMP for compound pages
	 */
	gfp_t gfp_mask = (GFP_ATOMIC | __GFP_COLD | __GFP_COMP);
	struct page *my_page;
	long int tmp = (long int) data;
	int order = tmp;
	int i;

	/* Drop WARN on failures, time_bench will invalidate test */
	gfp_mask |= __GFP_NOWARN;

	time_bench_start(rec);
	/** Loop to measure **/
	for (i = 0; i < rec->loops; i++) {
		my_page = alloc_pages(gfp_mask, order);
		if (unlikely(my_page == NULL))
			return 0;
		__free_pages(my_page, order);
	}
	time_bench_stop(rec, i);

	if (verbose) {
		time_bench_calc_stats(rec);
		pr_info("alloc_pages order:%d(%luB/x%d) %llu cycles"
			" per-%luB %llu cycles\n",
			order, PAGE_SIZE << order, 1 << order,
			rec->tsc_cycles, PAGE_SIZE,
			rec->tsc_cycles >> order);
	}

	return i;
}

int run_timing_tests(void)
{
	uint32_t loops = 100000;

	time_bench_loop(loops, 0, "single_page_alloc_put",
			NULL, time_single_page_alloc_put);

	time_bench_loop(loops, 0, "alloc_pages_order0",
			(void *)0, time_alloc_pages);
	time_bench_loop(loops, 0, "alloc_pages_order1",
			(void *)1, time_alloc_pages);
	time_bench_loop(loops, 0, "alloc_pages_order2",
			(void *)2, time_alloc_pages);
	time_bench_loop(loops, 0, "alloc_pages_order3",
			(void *)3, time_alloc_pages);
	time_bench_loop(loops, 0, "alloc_pages_order4",
			(void *)4, time_alloc_pages);
	time_bench_loop(loops, 0, "alloc_pages_order5",
			(void *)5, time_alloc_pages);
	return 0;
}

static int __init page_bench01_module_init(void)
{
	if (verbose)
		pr_info("Loaded\n");

#ifdef CONFIG_DEBUG_PREEMPT
	pr_warn("WARN: CONFIG_DEBUG_PREEMPT is enabled: this affect results\n");
#endif
	if (run_timing_tests() < 0) {
		return -ECANCELED;
	}

	return 0;
}
module_init(page_bench01_module_init);

static void __exit page_bench01_module_exit(void)
{
	if (verbose)
		pr_info("Unloaded\n");
}
module_exit(page_bench01_module_exit);

MODULE_DESCRIPTION("Benchmarking page alloactor execution time in kernel");
MODULE_AUTHOR("Jesper Dangaard Brouer <netoptimizer@brouer.com>");
MODULE_LICENSE("GPL");