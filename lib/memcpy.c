

void *__memcpy(void *_dst, const void *_src, unsigned len)
{
	unsigned char *dst = _dst;
	const unsigned char *src = _src;
	while(len-- > 0) {
		*dst++ = *src++;
	}
	return _dst;
}