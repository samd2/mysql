<xsl:stylesheet version="3.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xs="http://www.w3.org/2001/XMLSchema"
  exclude-result-prefixes="xs">

  <xsl:variable name="emphasized-template-parameter-types" select="
    'CompletionToken',
    'Stream',
    'Executor',
    'ValueCollection',
    'ValueForwardIterator'
  "/>

</xsl:stylesheet>
