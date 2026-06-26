<template>
  <div class="page-container">
    <div class="page-header">
      <h2>{{ isEdit ? 'Edit Maintenance' : 'Add Maintenance Record' }}</h2>
      <p>{{ isEdit ? 'Update maintenance information' : 'Record a new maintenance job' }}</p>
    </div>

    <el-card>
      <el-form
        ref="formRef"
        :model="form"
        :rules="rules"
        label-position="top"
        style="max-width: 560px"
      >
        <el-form-item v-if="!isEdit" label="Vehicle ID" prop="vehicle_id">
          <el-input-number v-model="form.vehicle_id" :min="1" style="width: 100%" placeholder="Vehicle ID" />
        </el-form-item>

        <el-form-item label="Type" prop="type">
          <el-select v-model="form.type" placeholder="Select type" style="width: 100%">
            <el-option label="Regular" value="regular" />
            <el-option label="Repair" value="repair" />
            <el-option label="Inspection" value="inspection" />
            <el-option label="Emergency" value="emergency" />
          </el-select>
        </el-form-item>

        <el-form-item label="Description" prop="description">
          <el-input v-model="form.description" type="textarea" :rows="3" placeholder="Describe the maintenance work" />
        </el-form-item>

        <div class="form-row">
          <el-form-item label="Cost (CNY)" style="flex: 1">
            <el-input-number v-model="form.cost" :min="0" :precision="2" style="width: 100%" />
          </el-form-item>
          <el-form-item label="Technician" style="flex: 1">
            <el-input v-model="form.technician" placeholder="Technician name" />
          </el-form-item>
        </div>

        <div class="form-row">
          <el-form-item label="Start Date" style="flex: 1">
            <el-date-picker v-model="form.start_date" type="date" placeholder="Start date" value-format="YYYY-MM-DD" style="width: 100%" />
          </el-form-item>
          <el-form-item v-if="isEdit" label="End Date" style="flex: 1">
            <el-date-picker v-model="form.end_date" type="date" placeholder="End date" value-format="YYYY-MM-DD" style="width: 100%" />
          </el-form-item>
        </div>

        <el-form-item v-if="isEdit" label="Status">
          <el-select v-model="form.status" placeholder="Select status" style="width: 100%">
            <el-option label="Pending" value="pending" />
            <el-option label="In Progress" value="in_progress" />
            <el-option label="Completed" value="completed" />
          </el-select>
        </el-form-item>

        <el-form-item>
          <el-button type="primary" :loading="submitting" @click="handleSubmit">
            {{ isEdit ? 'Save Changes' : 'Create Record' }}
          </el-button>
          <el-button @click="$router.back()">Cancel</el-button>
        </el-form-item>
      </el-form>
    </el-card>
  </div>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { createMaintenance, updateMaintenance, getMaintenanceList } from '@/api/maintenance'
import { ElMessage } from 'element-plus'

const route = useRoute()
const router = useRouter()
const isEdit = computed(() => !!route.params.id)
const submitting = ref(false)
const formRef = ref(null)

const form = reactive({
  vehicle_id: undefined,
  type: '',
  description: '',
  cost: 0,
  technician: '',
  start_date: '',
  end_date: '',
  status: 'pending',
})

const rules = {
  vehicle_id: [{ required: true, message: 'Vehicle ID is required', trigger: 'blur' }],
  type: [{ required: true, message: 'Type is required', trigger: 'change' }],
  description: [{ required: true, message: 'Description is required', trigger: 'blur' }],
}

async function loadRecord() {
  if (!isEdit.value) return
  try {
    const res = await getMaintenanceList({ id: route.params.id })
    if (res.error === 0) {
      const record = (res.list || []).find(r => String(r.id) === String(route.params.id))
      if (record) {
        Object.assign(form, {
          type: record.type || '',
          description: record.description || '',
          cost: record.cost || 0,
          technician: record.technician || '',
          start_date: record.start_date || '',
          end_date: record.end_date || '',
          status: record.status || 'pending',
        })
      }
    }
  } catch {
    ElMessage.error('Failed to load record')
  }
}

async function handleSubmit() {
  const valid = await formRef.value.validate().catch(() => false)
  if (!valid) return

  submitting.value = true
  try {
    let res
    if (isEdit.value) {
      res = await updateMaintenance({ id: Number(route.params.id), ...form })
    } else {
      res = await createMaintenance(form)
    }
    if (res.error === 0) {
      ElMessage.success(isEdit.value ? 'Record updated' : 'Record created')
      router.push('/maintenance')
    } else {
      ElMessage.error('Operation failed')
    }
  } catch {
    ElMessage.error('Operation failed')
  } finally {
    submitting.value = false
  }
}

onMounted(loadRecord)
</script>

<style scoped>
.form-row {
  display: flex;
  gap: 20px;
}
</style>
