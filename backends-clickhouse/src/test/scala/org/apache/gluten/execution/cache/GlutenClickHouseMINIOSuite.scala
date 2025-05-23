/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.gluten.execution.cache

import org.apache.hadoop.fs.{FileUtil, Path}

class GlutenClickHouseMINIOSuite extends GlutenClickHouseCacheBaseTestSuite {

  private val BUCKET = "tpch-data"
  override protected val tablesPath: String = s"s3a://$BUCKET"

  override protected def copyDataIfNeeded(): Unit = {
    val targetFile = new Path(s"$tablesPath/lineitem")
    val fs = targetFile.getFileSystem(spark.sessionState.newHadoopConf())
    val existed = fs.exists(targetFile)
    // If the 'lineitem' directory doesn't exist in Minio,
    // upload the 'lineitem' data from the local system.
    if (!existed) {
      val localDataDir = new Path(s"$absoluteParquetPath/lineitem")
      val localFs = localDataDir.getFileSystem(spark.sessionState.newHadoopConf())
      FileUtil.copy(
        localFs,
        localDataDir,
        fs,
        targetFile,
        false,
        true,
        spark.sessionState.newHadoopConf())
    }
  }

  override def beforeAll(): Unit = {
    if (!minioHelper.bucketExists(BUCKET)) {
      minioHelper.createBucket(BUCKET)
    }
    super.beforeAll()
  }
}
