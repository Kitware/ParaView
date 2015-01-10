<?xml version="1.0" encoding="utf8"?>
<!-- XSL used to add categoryindex node for Readers and Writers
    to run use : xmlpatterns <xsl> <xml> -output <xml>
-->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="xml"/>


<xsl:template match="/xml">
  <xml>

    <xsl:for-each select="proxy">
      <xsl:copy-of select="." />
    </xsl:for-each>

    <xsl:for-each select="categoryindex">
      <xsl:copy-of select="." />
    </xsl:for-each>

    <!-- TODO use a function for this -->
    <xsl:if test="count(proxy/ReaderFactory) > 0">
      <categoryindex>
        <label>Readers</label>
        <xsl:for-each select="proxy[ReaderFactory and group='sources']">
          <xsl:element name="item">
            <xsl:attribute name="proxy_name">
            <xsl:value-of select="name"/></xsl:attribute>

            <xsl:attribute name="proxy_group">
            <xsl:value-of select="group"/></xsl:attribute>
          </xsl:element>
        </xsl:for-each>
      </categoryindex>
    </xsl:if>

    <xsl:if test="count(proxy/WriterFactory) > 0">
      <categoryindex>
        <label>Writers</label>
        <xsl:for-each select="proxy[WriterFactory]">
          <xsl:element name="item">
            <xsl:attribute name="proxy_name">
            <xsl:value-of select="name"/></xsl:attribute>

            <xsl:attribute name="proxy_group">
            <xsl:value-of select="group"/></xsl:attribute>
          </xsl:element>
        </xsl:for-each>
      </categoryindex>
    </xsl:if>


  </xml>
</xsl:template>

</xsl:stylesheet>
