<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='html' version='1.0' encoding='UTF-8' indent='yes'/>

<xsl:template match="/">
  <html>
  <body>
<xsl:apply-templates/>
  </body>
  </html>
</xsl:template>

<xsl:template match="BuildInfo">
<h2>Build Info
Platform:<xsl:value-of select="./@platform"/>
Compiler:<xsl:value-of select="./@compiler"/>
</h2>
</xsl:template>

<xsl:template match="TestSuite">
<table border="1">
<tr bgcolor="#9acd32"><td>TestSuite:<xsl:value-of select="./@name"/></td></tr>
<tr>
<xsl:apply-templates/>
</tr>
</table>
</xsl:template>

<xsl:template match="TestCase">
<table border="1" width="100%">
<tr bgcolor="gray">
  <td>Test Case</td><td width="5%">Time</td><td width="10%">Status</td>
</tr>
<tr>
  <td><xsl:value-of select="./@name"/></td>
  <td><xsl:value-of select="TestingTime"/></td>
  <td><xsl:if test="*=FatalError"><font color="red"><b>FAILED</b></font></xsl:if></td>
</tr>
<tr>
<td colspan="3">
  <xsl:apply-templates/>
</td>
</tr>
</table>
  <!--xsl:for-each select="Message">
    <xsl:value-of select="file"/>#<xsl:value-of select="line"/>:<xsl:value-of select="."/>
  </xsl:for-each-->
</xsl:template>

<xsl:template match="TestCase/FatalError">
<table width="100%" border="2">
<tr><td width="100%">Location: <a><xsl:attribute name="href">https://10.16.0.73:8443/svn/svnroot/ISA100/SystemManager/trunk/SystemManager/<xsl:value-of select="@file" /></xsl:attribute><xsl:value-of select="@file"/></a>#<xsl:value-of select="@line"/></td></tr>
<tr><td><xsl:value-of select="."/></td></tr>
</table>
</xsl:template>

<xsl:template match="Message">
</xsl:template>

<xsl:template match="TestingTime">
</xsl:template>

</xsl:stylesheet>