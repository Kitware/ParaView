<?xml version="1.0" encoding="utf8"?>
<!-- Used to convert XML DOM generated from smxml_to_xml.xsl to HTML -->
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html"/>
<xsl:template match="/xml">
  <xsl:apply-templates select="proxy" />
  <xsl:apply-templates select="categoryindex" />
</xsl:template>

<xsl:template name="proxy_index">
   <xsl:param name="proxy_group" />
   <xsl:param name="proxy_name" />
   <xsl:param name="proxy_label" />
   <tr>
      <td>
         <xsl:element name="a">
            <xsl:attribute name="href"><xsl:value-of select="$proxy_group"/>.<xsl:value-of select="$proxy_name"/>.html</xsl:attribute>
            <xsl:value-of select="/xml/proxy[group=$proxy_group and name=$proxy_name]/label" />
            <span />
         </xsl:element>
      </td>
      <td>
         <xsl:value-of select="/xml/proxy[group=$proxy_group and name=$proxy_name]/documentation/brief" />
      </td>
      <xsl:if test="$proxy_label = 'Writers'">
        <td>
          <xsl:value-of select="/xml/proxy[group=$proxy_group and name=$proxy_name]/WriterFactory/@extensions" />
        </td>
      </xsl:if>
      <xsl:if test="$proxy_label = 'Readers'">
        <td>
          <xsl:value-of select="/xml/proxy[group=$proxy_group and name=$proxy_name]/ReaderFactory/@extensions" />
        </td>
      </xsl:if>
   </tr>
</xsl:template>

<xsl:template name="all_proxies_index">
  <xsl:param name="proxy_group" />
  <xsl:param name="proxy_label" />
  <!-- we select only proxies for this group,
       which also exist in /xml/categoryindex/item -->
  <xsl:for-each select="/xml/proxy[group=$proxy_group and name = /xml/categoryindex[label = $proxy_label]/item/@proxy_name]">
    <xsl:sort select="label"/>
    <xsl:call-template name="proxy_index">
      <xsl:with-param name="proxy_group"><xsl:value-of select="group"/>
      </xsl:with-param>
      <xsl:with-param name="proxy_name"><xsl:value-of select="name"/>
      </xsl:with-param>
      <xsl:with-param name="proxy_label"><xsl:value-of select="$proxy_label"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:for-each>
</xsl:template>

<xsl:template match="/xml/categoryindex">
  <html>
    <head>
      <title><xsl:value-of select="label" /> Index</title>
      <xsl:element name="meta">
        <xsl:attribute name="name">filename</xsl:attribute>
        <xsl:attribute name="contents"><xsl:value-of select="label"/>.html</xsl:attribute></xsl:element>
    </head>
    <body>
      <h2><xsl:value-of select="label" /></h2>
      <hr/>
      <table class="index_table">
         <tr><th>Name</th><th>Description</th>
         <xsl:if test="label = 'Readers' or label = 'Writers'">
           <th>Extension</th>
         </xsl:if>
         </tr>
         <xsl:call-template name="all_proxies_index">
           <xsl:with-param name="proxy_group">
             <xsl:value-of select="item[1]/@proxy_group"/>
           </xsl:with-param>
           <xsl:with-param name="proxy_label">
             <xsl:value-of select="label"/>
           </xsl:with-param>
         </xsl:call-template>
      </table>
    </body>
  </html>
</xsl:template>

<xsl:template match="/xml/proxy">
  <html>
    <head>
      <title><xsl:value-of select="label" /></title>
      <xsl:element name="meta">
        <xsl:attribute name="name">proxy_name</xsl:attribute>
        <xsl:attribute name="contents"><xsl:value-of select="group"/>.<xsl:value-of select="name" /></xsl:attribute>
      </xsl:element>
      <xsl:element name="meta">
        <xsl:attribute name="name">filename</xsl:attribute>
        <xsl:attribute name="contents"><xsl:value-of select="group"/>.<xsl:value-of select="name" />.html</xsl:attribute></xsl:element>
    </head>
    <body>
      <h2><xsl:value-of select="label"/> (<xsl:value-of select="name"/>)</h2>
      <i><p><xsl:value-of select="documentation/brief" /></p></i>
      <div class="description"><xsl:value-of select="documentation/long" /></div>
      <table width="97%" border="2px">
        <tr bgcolor="#9acd32">
          <th>Property</th>
          <th width="60%">Description</th>
          <th width="5%">Default(s)</th>
          <th width="20%">Restrictions</th>
        </tr>

        <xsl:for-each select="property">
          <tr>
          <th><xsl:value-of select="label" /></th>
          <td><xsl:value-of select="documentation/long" /></td>
          <td><xsl:value-of select="defaults" /><span/></td>
          <td>
            <xsl:for-each select="domains/domain">
              <p>
                <xsl:value-of select="text"/>
                <xsl:for-each select="list" >
                  <ul>
                    <xsl:for-each select="item">
                      <li><xsl:value-of select="."/></li>
                    </xsl:for-each>
                  </ul>
                </xsl:for-each>
              </p>
            </xsl:for-each>
          </td>
          </tr>
        </xsl:for-each>
      </table>
    </body>
  </html>
</xsl:template>

</xsl:stylesheet>
